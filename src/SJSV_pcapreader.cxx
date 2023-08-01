#include "SJSV_pcapreader.h"

SJSV_pcapreader::SJSV_pcapreader():
    _filename(""),
    is_reader_valid(false) {
}

SJSV_pcapreader::SJSV_pcapreader(std::string _filename_str):
    _filename(_filename_str),
    is_reader_valid(false) {
    if (_filename.empty()) {
        LOG(ERROR) << "Filename is empty";
        return;
    }
}

SJSV_pcapreader::~SJSV_pcapreader() {
    if (!_reader->open())
        _reader->close();
    if (_reader != NULL)
        delete _reader;
}

bool SJSV_pcapreader::read_pcapfile() {
    is_reader_valid = false;

    if (_filename.empty()) {
        LOG(ERROR) << "Filename is empty";
        return false;
    }

    _reader = pcpp::IFileReaderDevice::getReader(_filename);

    if (_reader == NULL) {
        LOG(ERROR) << "Cannot read file " << _filename;
        return false;
    }
    if (!_reader->open()) {
        LOG(ERROR) << "Cannot open file " << _filename;
        return false;
    }

    auto _packet_num        = 0;
    auto _eth_packet_num    = 0;
    auto _ip_packet_num     = 0;
    auto _udp_packet_num    = 0;
    auto _daq_packet_num    = 0;
    // Read the packets from the file
    pcpp::RawPacket rawPacket;
    while(_reader->getNextPacket(rawPacket)) {
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

    _reader->close();

    _reader = pcpp::IFileReaderDevice::getReader(_filename);

    if (_reader == NULL) {
        LOG(ERROR) << "Cannot read file " << _filename;
        return false;
    }
    if (!_reader->open()) {
        LOG(ERROR) << "Cannot open file " << _filename;
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

std::string SJSV_pcapreader::protocol2str(pcpp::ProtocolType protocolType) {
    switch (protocolType) {
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
