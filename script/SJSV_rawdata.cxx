#include <iostream>

#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"


void set_easylogger(); // set easylogging++ configurations

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();
    LOG(INFO) << "Start SJSV_rawdata.cxx";

    std::string filename_pcap = "../data/traffic_2023073101.pcap";

    // split filename according to '_'
    std::string filename_raw_root = "../tmp/raw_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";

    std::string filename_parsed_root = "../tmp/parsed_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";

    // * Test pcapreader
    SJSV_pcapreader pcapreader(filename_pcap);
    pcapreader.read_pcapfile();
    pcapreader.test_decode_first_packet();
    auto vec_len = pcapreader.full_decode_pcapfile();
    if (pcapreader.save_to_rootfile(filename_raw_root))
        LOG(INFO) << "Save to rootfile success";
    else
        LOG(ERROR) << "Save to rootfile fail";

    // * Test eventbuilder
    SJSV_eventbuilder eventbuilder;
    eventbuilder.load_raw_data(filename_raw_root);
    eventbuilder.set_bcid_cycle(25);
    eventbuilder.set_tdc_slope(60);
    eventbuilder.parse_raw_data();
    if (eventbuilder.save_parsed_data(filename_parsed_root))
        LOG(INFO) << "Save to rootfile success";
    else
        LOG(ERROR) << "Save to rootfile fail";
    return 0;
}

void set_easylogger(){
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime{%H:%m:%s}[%levshort] (%fbase) %msg");
    defaultConf.set(el::Level::Info,    el::ConfigurationType::Format, 
        "%datetime{%H:%m:%s}[\033[1;34m%levshort\033[0m] (%fbase) %msg");
    defaultConf.set(el::Level::Warning, el::ConfigurationType::Format, 
        "%datetime{%H:%m:%s}[\033[1;33m%levshort\033[0m] (%fbase) %msg");
    defaultConf.set(el::Level::Error,   el::ConfigurationType::Format, 
        "%datetime{%H:%m:%s}[\033[1;31m%levshort\033[0m] (%fbase) %msg");
    el::Loggers::reconfigureLogger("default", defaultConf);
}