#include "SJSV_pcapreader.h"

SJSV_pcapreader::SJSV_pcapreader():
    filename(""),
    is_reader_valid(false) {
}

SJSV_pcapreader::SJSV_pcapreader(std::string _filename_str):
    filename(_filename_str),
    is_reader_valid(false) {
    if (filename.empty()) {
        LOG(ERROR) << "Filename is empty";
        return;
    }
}

SJSV_pcapreader::~SJSV_pcapreader() {
    if (!reader->open())
        reader->close();
    if (reader != NULL)
        delete reader;
}

bool SJSV_pcapreader::read_pcapfile() {
    is_reader_valid = false;

    if (filename.empty()) {
        LOG(ERROR) << "Filename is empty";
        return false;
    }

    reader = pcpp::IFileReaderDevice::getReader(filename);

    if (reader == NULL) {
        LOG(ERROR) << "Cannot read file " << filename;
        return false;
    }
    if (!reader->open()) {
        LOG(ERROR) << "Cannot open file " << filename;
        return false;
    }

    auto _packet_num        = 0;
    auto _eth_packet_num    = 0;
    auto _ip_packet_num     = 0;
    auto _udp_packet_num    = 0;
    auto _daq_packet_num    = 0;
    // Read the packets from the file
    pcpp::RawPacket rawPacket;
    while(reader->getNextPacket(rawPacket)) {
        _packet_num++;
        pcpp::Packet parsedPacket(&rawPacket);

        pcpp::EthLayer* ethernetLayer = parsedPacket.getLayerOfType<pcpp::EthLayer>();
        if (ethernetLayer == NULL) {
            LOG(ERROR) << "Cannot find ethernet layer for packet #" << _packet_num;
        } else {
            _eth_packet_num++;
        }

        pcpp::IPv4Layer* ipLayer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
        if (ipLayer == NULL) {
            LOG(ERROR) << "Cannot find IPv4 layer for packet #" << _packet_num;
        } else {
            _ip_packet_num++;
        }

        pcpp::UdpLayer* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        if (udpLayer == NULL) {
            LOG(ERROR) << "Cannot find UDP layer for packet #" << _packet_num;
        } else {
            _udp_packet_num++;
            if (udpLayer->getSrcPort() == DAQ_DATA_SRC_PORT) {
                _daq_packet_num++;
            }
        }

    }

    reader->close();

    reader = pcpp::IFileReaderDevice::getReader(filename);

    if (reader == NULL) {
        LOG(ERROR) << "Cannot read file " << filename;
        return false;
    }
    if (!reader->open()) {
        LOG(ERROR) << "Cannot open file " << filename;
        return false;
    }

    LOG(INFO) << "Found " << _packet_num << " packets";
    LOG(INFO) << "Found " << _eth_packet_num << " ethernet packets";
    is_reader_valid = true;
    LOG(INFO) << "Found " << _ip_packet_num << " IPv4 packets";
    LOG(INFO) << "Found " << _udp_packet_num << " UDP packets";
    LOG(INFO) << "Found " << _daq_packet_num << " DAQ packets";
    return true;
}

std::string SJSV_pcapreader::protocol2str(pcpp::ProtocolType _protocolType) {
    switch (_protocolType) {
        case pcpp::Ethernet:
            return "Ethernet";
        case pcpp::IPv4:
            return "IPv4";
        case pcpp::TCP:
            return "TCP";
        case pcpp::UDP:
            return "UDP";
        case pcpp::HTTPRequest:
            return "HTTPRequest";
        case pcpp::HTTPResponse:
            return "HTTPResponse";
        default:
            return "Unknown";
    }
}

int SJSV_pcapreader::test_decode_first_packet(){
    if (!is_reader_valid) {
        LOG(ERROR) << "Reader is not valid";
        return -1;
    }

    uni_frame _first_frame;
    pcpp::RawPacket rawPacket;
    bool _found_daq_packet = false;
    int _packet_num = 0;

    while(_found_daq_packet == false) {
        if (!reader->getNextPacket(rawPacket)) {
            LOG(ERROR) << "Cannot find daq packet";
            return -1;
        }

        pcpp::Packet parsedPacket(&rawPacket);
        pcpp::UdpLayer* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        if (udpLayer != NULL) {
            if (udpLayer->getSrcPort() == DAQ_DATA_SRC_PORT) {
                _found_daq_packet = true;
                LOG(INFO) << "Found DAQ packet #" << _packet_num;
                _first_frame = this->decode_pcap_packet(parsedPacket)[0];
            }
        } 
        
        if (_found_daq_packet) {
            LOG(INFO) << "flag: " << _first_frame.flag_daq;
            LOG(INFO) << "adc: " << _first_frame.adc;
            LOG(INFO) << "channel: " << _first_frame.channel;
            LOG(INFO) << "timestamp: " << _first_frame.timestamp;
            LOG(INFO) << "found_daq_packet: " << _found_daq_packet;
        }
        _packet_num++;
    }

    return _packet_num;
}

std::vector<SJSV_pcapreader::uni_frame> SJSV_pcapreader::decode_pcap_packet(const pcpp::Packet &_parsedPacket) {
    std::vector<SJSV_pcapreader::uni_frame> _frame_array;

    auto _udpLayer = _parsedPacket.getLayerOfType<pcpp::UdpLayer>();
    if (_udpLayer == NULL) {
        LOG(ERROR) << "Cannot find UDP layer for packet";
        return _frame_array;
    }

    auto _payload = _udpLayer->getLayerPayload();
    if (_payload == NULL) {
        LOG(ERROR) << "Cannot find payload for packet";
        return _frame_array;
    }

    auto _payload_len = _udpLayer->getLayerPayloadSize();
    // LOG(DEBUG) << "Payload length: " << _payload_len;

    for (int i = LEN_RAW_HEADER_BYTE; i < _payload_len; i += LEN_RAW_FRAME_BYTE) {
        uni_frame _frame;
        uint32_t _data1 = _payload[i] << 24 | _payload[i+1] << 16 | _payload[i+2] << 8 | _payload[i+3];
        uint16_t _data2 = _payload[i+4] << 8 | _payload[i+5];

        _frame.flag_daq = (_data2 >> 15) & 0x1;
        if (_frame.flag_daq) {
            _frame.offset    = (_data1 >> 27) & 0x1F;
            _frame.vmm_id    = (_data1 >> 22) & 0x1F;
            _frame.adc       = (_data1 >> 12) & 0x3FF;
            _frame.bcid      = Gray2bin32(_data1 & 0xFFF);
            _frame.daqdata38 = (_data2 >> 14) & 0x1;
            _frame.channel   = (_data2 >> 8) & 0x3f;
            _frame.tdc       = _data2 & 0xFF;
            _frame.timestamp = 0;
        } else {
            _frame.offset    = 0;
            _frame.vmm_id    = 0;
            _frame.adc       = 0;
            _frame.bcid      = 0;
            _frame.daqdata38 = 0;
            _frame.channel   = 0;
            _frame.tdc       = 0;
            uint64_t timestamp_lower_10bit = _data2 & 0x03FF;
            uint64_t timestamp_upper_32bit = _data1;
            _frame.timestamp = (timestamp_upper_32bit << 10) + timestamp_lower_10bit;
        }
        _frame_array.push_back(_frame);
    }



    return _frame_array;
}

uint32_t SJSV_pcapreader::Gray2bin32(uint32_t _num) {
    _num = _num ^ (_num >> 16);
    _num = _num ^ (_num >> 8);
    _num = _num ^ (_num >> 4);
    _num = _num ^ (_num >> 2);
    _num = _num ^ (_num >> 1);
    return _num;
}
