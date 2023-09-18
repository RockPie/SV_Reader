#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "TVectorD.h"
#include "TF1.h"
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations


int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    int run_number = 37;
    bool save_to_rootfile = true;
    bool save_to_png = true;
    auto bcid_cycle     = uint8_t(25);
    auto tdc_slope      = uint8_t(60);
    Double_t reconstructed_threshold_time_ns = 10000;

    int opt;
    while ((opt = getopt(argc, argv, "r:")) != -1){
        switch (opt){
            case 'r':
                run_number = atoi(optarg);
                break;
            default:
                LOG(ERROR) << "Wrong arguments!";
                return 1;
        }
    }

    LOG(INFO) << "Run number: " << run_number;

    auto root_file_name = Form("../tmp/parsed_Run0%dv.root", run_number);
    auto export_file_name = Form("../tmp/ERcon_Run0%dv.root", run_number);
    std::string filename_mapping_csv = "../data/config/Mapping_tb2023Sep_VMM2.csv";
    auto hglg_file_name = Form("../tmp/HL_Corr_Run0%dv.root", run_number);

    SJSV_eventbuilder eventbuilder;
    eventbuilder.load_parsed_data(root_file_name);
    eventbuilder.load_mapping_file(filename_mapping_csv);
    eventbuilder.set_bcid_cycle(bcid_cycle);
    eventbuilder.set_tdc_slope(tdc_slope);
    eventbuilder.reconstruct_event_list(reconstructed_threshold_time_ns);

    auto hglg_file = new TFile(hglg_file_name, "READ");
    hglg_file->cd("HG_LG_Correlation");
    
    // read the TVectors from the file
    auto hglg_corr_slope = (TVectorD*)gDirectory->Get("hg_lg_corr_slope");
    auto hglg_corr_offset = (TVectorD*)gDirectory->Get("hg_lg_corr_intercept");
    auto hglg_corr_x = (TVectorD*)gDirectory->Get("hg_lg_corr_x");
    auto hglg_corr_y = (TVectorD*)gDirectory->Get("hg_lg_corr_y");

    LOG(DEBUG) << "Read the TVectors from the file.";

    std::vector<Double_t> hglg_corr_slope_vec;
    for (int i = 0; i < hglg_corr_slope->GetNoElements(); i++){
        hglg_corr_slope_vec.push_back((*hglg_corr_slope)[i]);
    }

    LOG(DEBUG) << "Slope vector transformed.";

    std::vector<Double_t> hglg_corr_offset_vec;
    for (int i = 0; i < hglg_corr_offset->GetNoElements(); i++){
        // LOG(DEBUG) << (*hglg_corr_offset)[i] << " " << i;
        hglg_corr_offset_vec.push_back((*hglg_corr_offset)[i]);
    }

    LOG(DEBUG) << "Offset vector transformed.";

    std::vector<Double_t> hglg_corr_x_vec;
    for (int i = 0; i < hglg_corr_x->GetNoElements(); i++){
        hglg_corr_x_vec.push_back((*hglg_corr_x)[i]);
    }

    LOG(DEBUG) << "X vector transformed.";

    std::vector<Double_t> hglg_corr_y_vec;
    for (int i = 0; i < hglg_corr_y->GetNoElements(); i++){
        hglg_corr_y_vec.push_back((*hglg_corr_y)[i]);
    }

    LOG(DEBUG) << "Y vector transformed.";

    std::vector<Double_t> hglg_cell_id;
    for (int i = 0; i < hglg_corr_x->GetNoElements(); i++){
        hglg_cell_id.push_back((*hglg_corr_x)[i] * 210 + (*hglg_corr_y)[i]);
    }

    LOG(DEBUG) << "Transformed TVectors to vectors.";

    auto root_file = new TFile(export_file_name, "RECREATE");
    root_file->cd();

    root_file->mkdir("Calibrated Hit Map");
    root_file->cd("Calibrated Hit Map");

    std::vector<Double_t> original_event_sum;
    std::vector<Double_t> calibrated_event_sum;
    auto event_count = eventbuilder.get_parsed_event_number();

    for (auto i=0; i<event_count; i++){
        
    }

    for (auto i=0; i<64; i++){
        auto event = eventbuilder.event_at(i);
        auto mapped_event = eventbuilder.map_event(event, *eventbuilder.get_mapping_info_ptr());
        auto calibrated_event_canvas = new TCanvas(Form("Calibrated Event %d", i), Form("Calibrated Event %d", i), 800, 600);

        auto event_map_calibrated = eventbuilder.plot_mapped_event_calib(mapped_event, hglg_cell_id, hglg_corr_slope_vec, hglg_corr_offset_vec);

        event_map_calibrated->GetXaxis()->SetTitle("X");
        event_map_calibrated->GetYaxis()->SetTitle("Y");

        event_map_calibrated->Draw("colz");

        calibrated_event_canvas->SaveAs(Form("../tmp/Calibrated_Event_%d.png", i));

        calibrated_event_canvas->Close();
    }

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