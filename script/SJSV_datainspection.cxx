#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();
    
    std::string script_info = "rcslrm";
    // r -- reduction of repeated hits in one event
    // m -- new mapping with VMM 0,1 and 4,5 swapped

    std::string filename_pcap = "../data/Run030";
    // std::string filename_pcap = "../data/RunTest1";
    // auto filename_id = filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1);
    // std::string filename_analysis_root = "../tmp/analysis_" + filename_id + ".root";
    auto filename_id = filename_pcap.substr(filename_pcap.find_last_of("/")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("/")-1) + "v";
    std::string filename_analysis_root = "../tmp/analysis_" + script_info + "_" + filename_id + ".root";
    // split filename according to '_'
    // std::string filename_raw_root = "../tmp/raw_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";
    // std::string filename_parsed_root = "../tmp/parsed_" + filename_pcap.substr(filename_pcap.find_last_of("_")+1, filename_pcap.find_last_of(".")-filename_pcap.find_last_of("_")-1) + ".root";

    std::string filename_raw_root = "../tmp/raw_" + filename_id + ".root";
    std::string filename_parsed_root = "../tmp/parsed_" + filename_id + ".root";
    std::string filename_mapping_csv = "../data/config/Mapping_tb2023Sep_VMM3.csv";

    int opt;
    while ((opt = getopt(argc, argv, "i:m:d:r:p:a:")) != -1){
        switch (opt){
            case 'i':
                script_info = std::string(optarg);
                break;
            case 'm':
                filename_mapping_csv = std::string(optarg);
                break;
            case 'd':
                filename_pcap = std::string(optarg);
                break;
            case 'r':
                filename_raw_root = std::string(optarg);
                break;
            case 'p':
                filename_parsed_root = std::string(optarg);
                break;
            case 'a':
                filename_analysis_root = std::string(optarg);
                break;
            default:
                LOG(ERROR) << "Wrong arguments!";
                return 1;
        }
    }

    LOG(INFO) << "script_info: " << script_info;
    LOG(INFO) << "filename_mapping_csv: " << filename_mapping_csv;
    LOG(INFO) << "filename_pcap: " << filename_pcap;
    LOG(INFO) << "filename_raw_root: " << filename_raw_root;
    LOG(INFO) << "filename_parsed_root: " << filename_parsed_root;
    LOG(INFO) << "filename_analysis_root: " << filename_analysis_root;
    
    
    bool save_to_rootfile = true;
    bool save_to_png = false;

    auto bcid_cycle     = uint8_t(25);
    auto tdc_slope      = uint8_t(60);
    Int_t bin_num       = 1024;
    Double_t bin_low    = 0;
    Double_t bin_high   = 1024;
    Int_t mapped_plot_cnt = 20;
    Double_t max_event_adc = 1024;
    Double_t AOI_Start = 0;
    Double_t AOI_End = 800000000;
    Double_t reconstructed_threshold_time_ns = 10000;
    Int_t vmm_num = 16;

    std::vector<uint16_t> interested_channels;
    for (auto i = 0; i < 64*vmm_num; i++) interested_channels.push_back(i);

    Int_t canvas_width = 1200;
    Int_t canvas_height = 1000;


    

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

    // * -------------------------------------------------------------------------------------------
    SJSV_eventbuilder eventbuilder;
    eventbuilder.load_mapping_file(filename_mapping_csv);
    eventbuilder.load_raw_data(filename_raw_root);
    eventbuilder.set_bcid_cycle(bcid_cycle);
    eventbuilder.set_tdc_slope(tdc_slope);
    eventbuilder.parse_raw_data();
    eventbuilder.reconstruct_event_list(reconstructed_threshold_time_ns);
    eventbuilder.show_first_event_info();
    LOG(INFO) << "Saving to parsed rootfile ...";
    if (eventbuilder.save_parsed_data(filename_parsed_root))
        LOG(INFO) << "Save to rootfile success";
    else
        LOG(ERROR) << "Save to rootfile fail";

    eventbuilder.show_first_event_info();
    
    // * -------------------------------------------------------------------------------------------

    auto analysis_file = new TFile(filename_analysis_root.c_str(), "RECREATE");
    // * -------------------------------------------------------------------------------------------

    // * -- Plot all channel hist --
    // * -------------------------------------------------------------------------------------------
    auto qp_canvas_all_hist = new TCanvas("qp_canvas_all_hist", "Quick plot", canvas_width, canvas_height);
    auto _all_hist = eventbuilder.quick_plot_multiple_channels_hist(interested_channels, bin_num, bin_low, bin_high);
    _all_hist->Draw("colz");
    qp_canvas_all_hist->SetGridx(2);
    if (save_to_png)
        qp_canvas_all_hist->SaveAs("../pics/quick_plot_all_hist.png");
    if (save_to_rootfile){
        analysis_file->cd();
        _all_hist->Write("all_hist");
    }
    qp_canvas_all_hist->Close();

    // Plot according to VMMs
    if (save_to_rootfile){
        analysis_file->mkdir("vmm_hist");
        analysis_file->cd("vmm_hist");
    }
    for (auto _vmm_index=0; _vmm_index<vmm_num; _vmm_index++){
        std::vector<uint16_t> vmm_interested_channels;
        for (auto _channel_index=0; _channel_index<64; _channel_index++){
            vmm_interested_channels.push_back(_vmm_index*64 + _channel_index);
        }
        auto qp_canvas_all_hist_vmm = new TCanvas(("qp_canvas_all_hist_vmm_" + std::to_string(_vmm_index)).c_str(), "Quick plot", canvas_width, canvas_height);
        auto _all_hist_vmm = eventbuilder.quick_plot_multiple_channels_hist(vmm_interested_channels, bin_num, bin_low, bin_high);
        _all_hist_vmm->SetTitle(("VMM " + std::to_string(_vmm_index)).c_str());
        _all_hist_vmm->Draw("colz");
        qp_canvas_all_hist_vmm->SetGridx(2);
        if (save_to_png)
            qp_canvas_all_hist_vmm->SaveAs(("../pics/quick_plot_all_hist_vmm_" + std::to_string(_vmm_index) + ".png").c_str());
        if (save_to_rootfile){
            _all_hist_vmm->Write(("all_hist_vmm_" + std::to_string(_vmm_index)).c_str());
        }
        qp_canvas_all_hist_vmm->Close();
        delete qp_canvas_all_hist_vmm;
    }
    // * -------------------------------------------------------------------------------------------
    eventbuilder.show_first_event_info();
    // * -- Plot reconstructed time --
    // * -------------------------------------------------------------------------------------------
    auto qb_canvas_time_index = new TCanvas("qb_canvas_time_index", "Quick browse time", canvas_width, canvas_height);
    auto qb_tgraph2 = eventbuilder.quick_plot_time_index(AOI_Start, AOI_End);
    qb_tgraph2->Draw("APL");
 
    qb_canvas_time_index->SetGrid();
    if (save_to_png)
        qb_canvas_time_index->SaveAs("../pics/quick_browse_time.png");

    // save to rootfile
    if (save_to_rootfile){
        analysis_file->cd();
        qb_tgraph2->Write("time_index");
    }
    qb_canvas_time_index->Close();
    // * -------------------------------------------------------------------------------------------

    // * -- Plot channel ADC histogram --
    // * -------------------------------------------------------------------------------------------
    auto qp_canvas_multi_ADC_hist = new TCanvas("qp_canvas", "Quick plot", canvas_width, canvas_height);

    std::vector<TH1D*> _vec_hist;
    auto valid_hist_cnt = 0;
    for (auto _channel : interested_channels) {
        auto _hist = eventbuilder.quick_plot_single_channel_hist(_channel, bin_num, bin_low, bin_high);
        valid_hist_cnt += (_hist == nullptr) ? 0 : 1;
        if (_hist == nullptr) {
            // LOG(WARNING) << "Channel " << _channel << " histogram is nullptr";
        }
        _vec_hist.push_back(_hist);
    }
    LOG(INFO) << "Total " << valid_hist_cnt << " valid channels";
    LOG(INFO) << "Total " << interested_channels.size() << " channels";

    for (auto i = 0; i < interested_channels.size(); i++){
        if (_vec_hist.at(i) == nullptr) continue;
        _vec_hist.at(i)->SetLineColor(kBlue);
        _vec_hist.at(i)->SetLineWidth(2);
        _vec_hist.at(i)->SetStats(true);
        // add fill color
        _vec_hist.at(i)->SetFillColor(kBlue-10);
        _vec_hist.at(i)->GetXaxis()->SetTitle("ADC");
        _vec_hist.at(i)->GetYaxis()->SetTitle("Counts");
        _vec_hist.at(i)->Draw("same");
    }

    auto _legend2 = new TLegend(0.7, 0.7, 0.9, 0.9);
    _legend2->Draw();
    for (auto i = 0; i < interested_channels.size(); i++){
        if (_vec_hist.at(i) == nullptr) continue;
        _legend2->AddEntry(_vec_hist.at(i), ("Channel " + std::to_string(interested_channels[i])).c_str(), "l");
    }

    qp_canvas_multi_ADC_hist->SetLogy();
    if (save_to_png)
        qp_canvas_multi_ADC_hist->SaveAs("../pics/quick_plot_hist.png");
    if (save_to_rootfile){
        analysis_file->cd();
        // create subfolder
        analysis_file->mkdir("channel_hist");
        analysis_file->cd("channel_hist");

        for (auto i = 0; i < interested_channels.size(); i++){
            if (_vec_hist.at(i) == nullptr) continue;
            _vec_hist.at(i)->Write(("channel_" + std::to_string(interested_channels[i])).c_str());
        }
    }
    qp_canvas_multi_ADC_hist->Close();
    for (auto _hist : _vec_hist) {
        if (_hist != nullptr) delete _hist;
    }
    // * -------------------------------------------------------------------------------------------

    // * -- Plot event channel count --
    // * -------------------------------------------------------------------------------------------
    auto qp_canvas_event_chnnum = new TCanvas("qp_canvas_event_chnnum", "Quick plot", canvas_width, canvas_height);
    auto _event_chnnum_hist = eventbuilder.quick_plot_event_chnnum_hist(interested_channels.size()*1.2);

    _event_chnnum_hist->SetLineColor(kBlue);
    _event_chnnum_hist->SetLineWidth(2);

    _event_chnnum_hist->SetFillColor(kBlue-10);

    _event_chnnum_hist->Draw();

    if(save_to_png)
        qp_canvas_event_chnnum->SaveAs("../pics/quick_plot_event_chnnum.png");
    if (save_to_rootfile){
        analysis_file->cd();
        _event_chnnum_hist->Write("event_chnnum_hist");
    }
    qp_canvas_event_chnnum->Close();
    // * -------------------------------------------------------------------------------------------

    // * -- Plot event ADC histogram --
    // * -------------------------------------------------------------------------------------------
    auto qp_canvas_event_adc = new TCanvas("qp_canvas_event_adc", "Quick plot", canvas_width, canvas_height);
    auto _event_adc_hist = eventbuilder.quick_plot_event_adc_hist(2000, 0, 50000);

    _event_adc_hist->SetLineColor(kBlue);
    _event_adc_hist->SetLineWidth(2);

    _event_adc_hist->SetFillColor(kBlue-10);

    _event_adc_hist->Draw();

    if(save_to_png)
        qp_canvas_event_adc->SaveAs("../pics/quick_plot_event_adc.png");
    if (save_to_rootfile){
        analysis_file->cd();
        _event_adc_hist->Write("event_adc_hist");
    }
    qp_canvas_event_adc->Close();
    // * -------------------------------------------------------------------------------------------


    // * -- Plot event LG ADC histogram --
    // * -------------------------------------------------------------------------------------------
    // eventbuilder.check_uni_channels("event_LG_adc");
    auto qp_canvas_event_LG_adc = new TCanvas("qp_canvas_event_LG_adc", "Quick plot", canvas_width, canvas_height);
    auto _event_LG_adc_hist = eventbuilder.quick_plot_event_LG_adc_hist(200, 0, 10000);

    _event_LG_adc_hist->SetLineColor(kBlue);
    _event_LG_adc_hist->SetLineWidth(2);

    _event_LG_adc_hist->SetFillColor(kBlue-10);

    _event_LG_adc_hist->Draw();

    if(save_to_png)
        qp_canvas_event_LG_adc->SaveAs("../pics/quick_plot_event_LG_adc.png");
    if (save_to_rootfile){
        analysis_file->cd();
        _event_LG_adc_hist->Write("event_LG_adc_hist");
    }
    qp_canvas_event_LG_adc->Close();

    // * -- Plot mapped events sum --
    // * -------------------------------------------------------------------------------------------
    // eventbuilder.check_uni_channels("mapped_sum");
    auto qp_canvas_mapped_events_sum = new TCanvas("qp_canvas_mapped_events_sum", "Quick plot", canvas_width, canvas_height);
    qp_canvas_event_chnnum->cd();
    auto _mapped_events_sum = eventbuilder.quick_plot_mapped_events_sum2();

    _mapped_events_sum->Draw("colz");


    if(save_to_png)
        qp_canvas_mapped_events_sum->SaveAs("../pics/quick_plot_mapped_events_sum.png");
    if (save_to_rootfile){
        analysis_file->cd();
        qp_canvas_mapped_events_sum->Write("mapped_events_sum");
    }
    qp_canvas_mapped_events_sum->Close();

    // * -------------------------------------------------------------------------------------------


    // * -- Plot mapped events --
    // * -------------------------------------------------------------------------------------------
    // eventbuilder.check_uni_channels("mapped_events");
    std::vector<TCanvas*> _vec_mapped_plots;
    if (save_to_rootfile){
        analysis_file->mkdir("mapped_events");
        analysis_file->cd("mapped_events");
    }
    for (auto i = 0; i < mapped_plot_cnt; i++) {
        auto qp_canvas_mapped_events = new TCanvas(("qp_canvas_mapped_events_" + std::to_string(i)).c_str(), "Quick plot", canvas_width, canvas_height);
        auto _mapped_event = eventbuilder.map_event(eventbuilder.event_at(i), *eventbuilder.get_mapping_info_ptr());
        auto _2d_hist = eventbuilder.quick_plot_mapped_event(_mapped_event, max_event_adc);
        qp_canvas_mapped_events->cd();
        _2d_hist->Draw("colz");
        _vec_mapped_plots.push_back(qp_canvas_mapped_events);

        if (save_to_png)
            qp_canvas_mapped_events->SaveAs(("../pics/quick_plot_mapped_events_" + std::to_string(i) + ".png").c_str());
        if (save_to_rootfile){
            qp_canvas_mapped_events->Write(("mapped_events_" + std::to_string(i)).c_str());
        }
        // avoid changing palette
        delete _2d_hist;
        qp_canvas_mapped_events->Close();
        delete qp_canvas_mapped_events;
    }
    // * -------------------------------------------------------------------------------------------

    // * save event energy to rootfile
    std::vector<Double_t> _vec_event_HG_adc_list = eventbuilder.get_event_adc_sum(true);
    std::vector<Double_t> _vec_event_LG_adc_list = eventbuilder.get_event_adc_sum(false);

    if (save_to_rootfile){
        analysis_file->mkdir("event_adc");
        analysis_file->cd("event_adc");
        analysis_file->mkdir("HG");
        analysis_file->cd("HG");

        TTree* _tree_event_HG_adc = new TTree("event_HG_adc", "event_HG_adc");
        _tree_event_HG_adc->Branch("event_HG_adc", &_vec_event_HG_adc_list);
        _tree_event_HG_adc->Fill();
        _tree_event_HG_adc->Write();


        analysis_file->cd("..");
        analysis_file->mkdir("LG");
        analysis_file->cd("LG");
        
        TTree* _tree_event_LG_adc = new TTree("event_LG_adc", "event_LG_adc");
        _tree_event_LG_adc->Branch("event_LG_adc", &_vec_event_LG_adc_list);
        _tree_event_LG_adc->Fill();
        _tree_event_LG_adc->Write();

    }



    analysis_file->Close();
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