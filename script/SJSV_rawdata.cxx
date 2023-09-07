#include <iostream>
#include "TCanvas.h" 
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations


int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    std::string filename_pcap = "../data/traffic_2023082102.pcap";
    auto bcid_cycle     = uint8_t(25);
    auto tdc_slope      = uint8_t(60);
    auto AOI_Start      =  10'000'000;
    auto AOI_End        = 200'000'000;
    Int_t bin_num       = 50;
    Double_t bin_low    = 200;
    Double_t bin_high   = 350;
    auto qp_channel_vec_adc  = std::vector<uint16_t>{64, 65, 66, 67, 68, 69, 70, 71};
    auto qp_channel_vec_hist = std::vector<uint16_t>{74, 75, 76, 77, 78, 79};

    // split filename according to '_'
    std::string filename_raw_root = "../tmp/raw_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";
    std::string filename_parsed_root = "../tmp/parsed_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";
    std::string filename_mapping_csv = "../data/config/Mapping_tb2023SPS.csv";

    // ! Create PCAP reader and read PCAP file into raw rootfile
    // * -------------------------------------------------------------------------------------------
    SJSV_pcapreader pcapreader(filename_pcap);
    pcapreader.read_pcapfile();
    pcapreader.test_decode_first_packet();
    auto vec_len = pcapreader.full_decode_pcapfile();
    LOG(INFO) << "Saving to raw rootfile ...";
    if (pcapreader.save_to_rootfile(filename_raw_root))
        LOG(INFO) << "Save to rootfile success";
    else
        LOG(ERROR) << "Save to rootfile fail";
    // * -------------------------------------------------------------------------------------------

    // ! Create event builder and read raw rootfile into parsed rootfile
    // * -------------------------------------------------------------------------------------------
    SJSV_eventbuilder eventbuilder;
    eventbuilder.load_mapping_file(filename_mapping_csv);
    eventbuilder.load_raw_data(filename_raw_root);
    eventbuilder.set_bcid_cycle(bcid_cycle);
    eventbuilder.set_tdc_slope(tdc_slope);
    eventbuilder.parse_raw_data();
    LOG(INFO) << "Saving to parsed rootfile ...";
    if (eventbuilder.save_parsed_data(filename_parsed_root))
        LOG(INFO) << "Save to rootfile success";
    else
        LOG(ERROR) << "Save to rootfile fail";
    // * -------------------------------------------------------------------------------------------
    
    auto _temp_pedestal = eventbuilder.load_pedestal_csv("../data/config/Pedestal_153653.csv");
    auto _temp_simple_pedestal = eventbuilder.get_simple_pedestal();
    eventbuilder.update_pedestal(_temp_simple_pedestal);
    // eventbuilder.enable_pedestal_subtraction(true);

    // * -- Plot reconstructed time --
    auto qb_canvas_time_index = new TCanvas("qb_canvas_time_index", "Quick browse time", 1200, 1000);
    auto qb_tgraph2 = eventbuilder.quick_plot_time_index(AOI_Start, AOI_End);
    qb_tgraph2->Draw("APL");
 
    qb_canvas_time_index->SetGrid();
    qb_canvas_time_index->SaveAs("../pics/quick_browse_time.png");
    qb_canvas_time_index->Close();

    // * -- Plot multichannel ADC --
    auto qb_canvas_multi_ADC = new TCanvas("qb_canvas", "Quick browse", 1200, 1000);

    auto qb_tgraph = eventbuilder.quick_plot_multiple_channels(qp_channel_vec_adc, AOI_Start, AOI_End);
    qb_tgraph->Draw("APL");
    auto _legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    for (auto i = 0; i < qp_channel_vec_adc.size(); i++)
        _legend->AddEntry(qb_tgraph->GetListOfGraphs()->At(i), ("Channel " + std::to_string(qp_channel_vec_adc[i])).c_str(), "l");
    _legend->Draw();
    qb_canvas_multi_ADC->SetGrid();
    qb_canvas_multi_ADC->SaveAs("../pics/quick_browse_multichn.png");
    qb_canvas_multi_ADC->Close();

    // * -- Plot reconstructed ADC histogram --
    auto qp_canvas_multi_ADC_hist = new TCanvas("qp_canvas", "Quick plot", 1200, 1000);

    std::vector<TH1D*> _vec_hist;
    for (auto _channel : qp_channel_vec_hist) {
        auto _hist = eventbuilder.quick_plot_single_channel_hist(_channel, bin_num, bin_low, bin_high);
        _vec_hist.push_back(_hist);
    }

    for (auto i = 0; i < qp_channel_vec_hist.size(); i++){
        _vec_hist.at(i)->SetLineColor(i+1);
        _vec_hist.at(i)->SetLineWidth(2);
        _vec_hist.at(i)->SetStats(0);
        _vec_hist.at(i)->GetXaxis()->SetTitle("ADC");
        _vec_hist.at(i)->GetYaxis()->SetTitle("Counts");
        _vec_hist.at(i)->Draw("same");
    }

    auto _legend2 = new TLegend(0.7, 0.7, 0.9, 0.9);
    for (auto i = 0; i < qp_channel_vec_hist.size(); i++)
        _legend2->AddEntry(_vec_hist.at(i), ("Channel " + std::to_string(qp_channel_vec_hist[i])).c_str(), "l");
    _legend2->Draw();

    qp_canvas_multi_ADC_hist->SetLogy();
    qp_canvas_multi_ADC_hist->SaveAs("../pics/quick_plot_hist.png");
    qp_canvas_multi_ADC_hist->Close();

    for (auto _hist : _vec_hist) delete _hist;
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