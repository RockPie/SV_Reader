#include <iostream>

#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

#include "TCanvas.h" 


void set_easylogger(); // set easylogging++ configurations

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();
    LOG(INFO) << "Start SJSV_rawdata.cxx";

    std::string filename_pcap = "../data/traffic_2023082102.pcap";

    // split filename according to '_'
    std::string filename_raw_root = "../tmp/raw_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";

    std::string filename_parsed_root = "../tmp/parsed_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";

    // * Test pcapreader
    SJSV_pcapreader pcapreader(filename_pcap);
    // * Run a single frame test
    // std::vector<uint8_t> frame = std::vector<uint8_t>{0x53, 0x70, 0x21, 0xdf, 0xd8, 0x62};
    // auto res_str = pcapreader.test_single_frame_decode(frame);
    // LOG(INFO) << "Single frame decode result: " << res_str;
    auto AOI_Start = 100'000'000;
    auto AOI_End   = 200'000'000;

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

    auto _temp_pedestal = eventbuilder.load_pedestal_csv("../data/config/Pedestal_153653.csv");

    auto _temp_simple_pedestal = eventbuilder.get_simple_pedestal();
    eventbuilder.update_pedestal(_temp_simple_pedestal);
    // eventbuilder.enable_pedestal_subtraction(true);

    auto qb_canvas2 = new TCanvas("qb_canvas2", "Quick browse2", 1200, 1000);
    auto qb_tgraph2 = eventbuilder.quick_plot_time_index(AOI_Start, AOI_End);
    qb_tgraph2->Draw("APL");
    // add grid
    qb_canvas2->SetGrid();
    qb_canvas2->SaveAs("../pics/quick_browse_time.png");
    qb_canvas2->Close();

    auto qb_canvas = new TCanvas("qb_canvas", "Quick browse", 1200, 1000);
    // set title
    // auto qb_tgraph = eventbuilder.quick_plot_single_channel(4, 50000000, 400000000);
    // auto qb_tgraph = eventbuilder.quick_plot_time_index(0, 700'000'000);
    auto qp_channel_vec = std::vector<uint16_t>{64, 65, 66, 67, 68, 69, 70, 71};
    auto qb_tgraph = eventbuilder.quick_plot_multiple_channels(qp_channel_vec, AOI_Start, AOI_End);
    qb_tgraph->Draw("APL");
    auto _legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    for (auto i = 0; i < qp_channel_vec.size(); i++){
        _legend->AddEntry(qb_tgraph->GetListOfGraphs()->At(i), ("Channel " + std::to_string(qp_channel_vec[i])).c_str(), "l");
    }
    _legend->Draw();
    qb_canvas->SetGrid();
    // set transparent background
    qb_canvas->SaveAs("../pics/quick_browse_multichn.png");
    qb_canvas->Close();

    auto qp_canvas = new TCanvas("qp_canvas", "Quick plot", 1200, 1000);
    auto qp_hist1 = eventbuilder.quick_plot_single_channel_hist(74);
    auto qp_hist2 = eventbuilder.quick_plot_single_channel_hist(75);
    auto qp_hist3 = eventbuilder.quick_plot_single_channel_hist(76);
    auto qp_hist4 = eventbuilder.quick_plot_single_channel_hist(77);

    qp_hist1->SetLineColor(kRed);
    qp_hist2->SetLineColor(kBlue);
    qp_hist3->SetLineColor(kGreen);
    qp_hist4->SetLineColor(kBlack);

    qp_hist1->SetLineWidth(2);
    qp_hist2->SetLineWidth(2);
    qp_hist3->SetLineWidth(2);
    qp_hist4->SetLineWidth(2);

    qp_hist1->SetBins(100, 100, 512);
    qp_hist2->SetBins(100, 100, 512);
    qp_hist3->SetBins(100, 100, 512);
    qp_hist4->SetBins(100, 100, 512);

    qp_hist1->Draw("");
    qp_hist2->Draw("same");
    qp_hist3->Draw("same");
    qp_hist4->Draw("same");

    auto _legend2 = new TLegend(0.7, 0.7, 0.9, 0.9);
    _legend2->AddEntry(qp_hist1, "Channel 74", "l");
    _legend2->AddEntry(qp_hist2, "Channel 75", "l");
    _legend2->AddEntry(qp_hist3, "Channel 76", "l");
    _legend2->AddEntry(qp_hist4, "Channel 77", "l");
    _legend2->Draw();

    qp_canvas->SaveAs("../pics/quick_plot_hist.png");
    qp_canvas->Close();

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