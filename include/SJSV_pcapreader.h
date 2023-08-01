#pragma once

#include "easylogging++.h"

#include "stdlib.h"
#include "PcapFileDevice.h" // for .pcap file reading
#include "SystemUtils.h" // for .pcap file reading
#include "Packet.h" // for .pcap file readings
#include "EthLayer.h" // for .pcap file readings
#include "IPv4Layer.h" // for .pcap file readings
#include "TcpLayer.h" // for .pcap file readings
#include "UdpLayer.h" // for .pcap file readings
#include "HttpLayer.h" // for .pcap file readings\

#include "TDataType.h" // for .root file reading
#include "TFile.h" // for .root file reading
#include "TTree.h" // for .root file reading

#define DAQ_DATA_SRC_PORT   6006
#define ESS_SC_SRC_PORT     65535
#define FEC_SRC_PORT        6007
#define DVM_I2C_SRC_PORT    6601
#define VMMASIC_SRC_PORT    6603
#define VMMAPP_SRC_PORT     6600
#define S6_SRC_PORT         6602
#define I2C_SRC_PORT        6604
#define FEC_SYS_SRC_PORT    6023

#define LEN_RAW_FRAME_BYTE  6
#define LEN_RAW_HEADER_BYTE 16

INITIALIZE_EASYLOGGINGPP

class SJSV_pcapreader {
    public:
        struct uni_frame{
            bool        flag_daq;   // 1 bit, 1 - daq, 0 - timestamp
            uint8_t     offset;     // 5 bits
            uint8_t     vmm_id;     // 5 bits
            uint16_t    adc;        // 10 bits
            uint16_t    bcid;       // 12 bits
            bool        daqdata38;  // 1 bit
            uint8_t     channel;    // 6 bits
            uint8_t     tdc;        // 8 bits
            uint64_t    timestamp;  // 40 bits
        };
    public:
        SJSV_pcapreader();
        SJSV_pcapreader(std::string _filename_str);
        ~SJSV_pcapreader();

    public:
        inline void set_filename(std::string _filename_str) {
            if (_filename_str.empty()) {
                LOG(ERROR) << "Filename is empty";
                return;
            }
            if (this->is_reader_valid && reader != nullptr) {
                reader->close();
                delete reader;
                this->is_reader_valid = false;
            }
            if (this->is_uniframe_vec_valid){
                this->uni_frame_vec->clear();
                this->is_uniframe_vec_valid = false;
            }
            filename = _filename_str; 
        }

        // * Read .pcap file
        // * @return true if success, false if fail
        bool read_pcapfile();

        // * Test decoding the first packet
        // * @return -1 if fail, otherwise the index of the first daq packet
        int test_decode_first_packet();

        // * Decode .pcap packet
        // * @return uni_frame
        std::vector<uni_frame> decode_pcap_packet(const pcpp::Packet &_parsedPacket);
        inline std::vector<uni_frame> decode_pcap_packet(pcpp::RawPacket _rawPacket){
            pcpp::Packet parsedPacket(&_rawPacket);
            return decode_pcap_packet(parsedPacket);
        }

        // * Decode .pcap file
        // * @return -1 if fail, otherwise the length of the vector
        int64_t full_decode_pcapfile();

        bool save_to_rootfile(const std::string &_rootfilename);

    private:
        // * Convert protocol type to string
        std::string protocol2str(pcpp::ProtocolType _protocol);

        // * Convert gray code to binary
        uint32_t Gray2bin32(uint32_t _num);

    private:
        bool is_reader_valid;
        bool is_uniframe_vec_valid;

        std::string filename;
        pcpp::IFileReaderDevice* reader;

        std::vector<uni_frame>* uni_frame_vec;
};