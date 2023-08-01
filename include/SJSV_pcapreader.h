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

#define DAQ_DATA_SRC_PORT   6006
#define ESS_SC_SRC_PORT     65535
#define FEC_SRC_PORT        6007
#define DVM_I2C_SRC_PORT    6601
#define VMMASIC_SRC_PORT    6603
#define VMMAPP_SRC_PORT     6600
#define S6_SRC_PORT         6602
#define I2C_SRC_PORT        6604
#define FEC_SYS_SRC_PORT    6023

INITIALIZE_EASYLOGGINGPP

class SJSV_pcapreader {
    public:

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
            _filename = _filename_str; 
        }

        // * Read .pcap file
        // * @return true if success, false if fail
        bool read_pcapfile();

    private:
        std::string protocol2str(pcpp::ProtocolType protocol);

    private:
        bool is_reader_valid;

        std::string _filename;
        pcpp::IFileReaderDevice* _reader;
};