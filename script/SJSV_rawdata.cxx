#include <iostream>

#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

#include "TGraph.h"
#include "TCanvas.h" 


void set_easylogger(); // set easylogging++ configurations

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();
    LOG(INFO) << "Start SJSV_rawdata.cxx";

    std::string filename_pcap = "../data/traffic_2023080201.pcap";

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



    auto qb_canvas2 = new TCanvas("qb_canvas2", "Quick browse2", 1200, 1000);
    auto qb_tgraph2 = eventbuilder.quick_plot_time_index(100'000'000, 400'000'000);
    qb_tgraph2->Draw("APL");
    qb_canvas2->SaveAs("../pics/quick_browse_time.png");
    qb_canvas2->Close();

    auto qb_canvas = new TCanvas("qb_canvas", "Quick browse", 1200, 1000);
    // auto qb_tgraph = eventbuilder.quick_plot_single_channel(4, 50000000, 400000000);
    // auto qb_tgraph = eventbuilder.quick_plot_time_index(0, 700'000'000);
    auto qp_channel_vec = std::vector<uint16_t>{0, 1, 2, 3, 4, 5, 6, 7, 100, 101, 120};
    auto qb_tgraph = eventbuilder.quick_plot_multiple_channels(qp_channel_vec, 100'000'000, 400'000'000);
    qb_tgraph->Draw("APL");
    auto _legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    for (auto i = 0; i < qp_channel_vec.size(); i++){
        _legend->AddEntry(qb_tgraph->GetListOfGraphs()->At(i), ("Channel " + std::to_string(qp_channel_vec[i])).c_str(), "l");
    }
    _legend->Draw();
    qb_canvas->SaveAs("../pics/quick_browse_multichn.png");
    qb_canvas->Close();

    


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