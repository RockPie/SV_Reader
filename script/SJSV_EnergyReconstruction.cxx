#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "TVectorD.h"
#include "TF1.h"
#include "TLatex.h"
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations


int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    int run_number = 28;
    bool save_to_rootfile = true;
    bool save_detail_to_png = false;
    bool save_comparison_to_png = true;
    auto bcid_cycle     = uint8_t(25);
    auto tdc_slope      = uint8_t(60);
    Double_t reconstructed_threshold_time_ns = 10000;
    Double_t hist_bin_number = 1000;
    Double_t hist_bin_min = 0;
    Double_t hist_bin_max = 14000;

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
    // auto hglg_file_name = Form("../tmp/HL_Correlation/HL_Corr_Run0%dv.root", run_number);
    auto hglg_file_name = "../tmp/HL_Correlation/HL_Corr_Run037v.root";

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
    auto progress_divider = event_count / 10;

    for (auto i=0; i<event_count; i++){
        if (i % progress_divider == 0){
            LOG(INFO) << "Processing event " << i << " / " << event_count;
        }
        auto event = eventbuilder.event_at(i);
        auto event_adc_sum_hg = eventbuilder.get_event_hg_sum(event);
        auto mapped_event = eventbuilder.map_event(event, *eventbuilder.get_mapping_info_ptr());

        if (save_detail_to_png){
            auto calibrated_event_canvas = new TCanvas(Form("Calibrated Event %d", i), Form("Calibrated Event %d", i), 800, 600);

            auto event_map_calibrated = eventbuilder.plot_mapped_event_calib(mapped_event, hglg_cell_id, hglg_corr_slope_vec, hglg_corr_offset_vec);

            event_map_calibrated->GetXaxis()->SetTitle("X");
            event_map_calibrated->GetYaxis()->SetTitle("Y");

            event_map_calibrated->Draw("colz");

            calibrated_event_canvas->SaveAs(Form("../tmp/Calibrated_Event_%d.png", i));

            calibrated_event_canvas->Close();
        }
        
        original_event_sum.push_back(event_adc_sum_hg);
        calibrated_event_sum.push_back(eventbuilder.get_saturation_calib_sum(mapped_event, hglg_cell_id, hglg_corr_slope_vec, hglg_corr_offset_vec, 900));
    }

    auto energy_distribution_canvas = new TCanvas("Energy Distribution", "Energy Distribution", 800, 600);
    auto original_energy_distribution = new TH1D("Original Energy Distribution", "Original Energy Distribution", hist_bin_number, hist_bin_min, hist_bin_max);
    auto calibrated_energy_distribution = new TH1D("Calibrated Energy Distribution", "Calibrated Energy Distribution", hist_bin_number, hist_bin_min, hist_bin_max);

    for (auto i=0; i<original_event_sum.size(); i++){
        original_energy_distribution->Fill(original_event_sum[i]);
    }

    for (auto i=0; i<calibrated_event_sum.size(); i++){
        calibrated_energy_distribution->Fill(calibrated_event_sum[i]);
    }

    TLegend* legend = new TLegend(0.1, 0.7, 0.35, 0.9);
    legend->SetTextSize(0.035);
    legend->AddEntry(original_energy_distribution, "High Gain Only", "l");
    legend->AddEntry(calibrated_energy_distribution, "Mixed", "l");

    // normalize
    original_energy_distribution->Scale(1/original_energy_distribution->Integral());
    calibrated_energy_distribution->Scale(1/calibrated_energy_distribution->Integral());

    original_energy_distribution->SetLineColor(kBlue);
    calibrated_energy_distribution->SetLineColor(kRed);

    original_energy_distribution->SetLineWidth(2);
    calibrated_energy_distribution->SetLineWidth(2);

    original_energy_distribution->GetXaxis()->SetTitle("ADC");
    original_energy_distribution->GetYaxis()->SetTitle("Normalized Counts");

    original_energy_distribution->SetTitle("");
    calibrated_energy_distribution->SetTitle("");

    original_energy_distribution->GetYaxis()->SetTitleOffset(1.6);

    calibrated_energy_distribution->GetYaxis()->SetTitleOffset(1.6);

    // change stats box to top left
    original_energy_distribution->SetStats(0);
    calibrated_energy_distribution->SetStats(0);

    original_energy_distribution->Draw();
    calibrated_energy_distribution->Draw("same");
    legend->Draw();

    TLatex *latexInfo = new TLatex();
    latexInfo->SetTextSize(0.03);
    latexInfo->SetTextAlign(13);  //align at top
    latexInfo->DrawLatexNDC(0.12, 0.68, Form("Run 0%d", run_number));
    latexInfo->DrawLatexNDC(0.12, 0.64, Form("Hadron Beam, FoCal-H Prototype 2"));
    latexInfo->DrawLatexNDC(0.12, 0.60, Form("Septemper 2023 Test Beam"));
    latexInfo->SetTextSize(0.035);
    latexInfo->SetTextAlign(33);  //align at top left
    latexInfo->DrawLatexNDC(0.9, 0.88, Form("Original Entries = %d", int(original_energy_distribution->GetEntries())));

    latexInfo->DrawLatexNDC(0.9, 0.82, Form("Calibrated Entries = %d", int(calibrated_energy_distribution->GetEntries())));

    energy_distribution_canvas->SetGrid();

    if(save_comparison_to_png){
        energy_distribution_canvas->SaveAs(Form("../pics/EnergyRecon/energy_distribution_run%d.png", run_number));
    }
    energy_distribution_canvas->Close();

    // save original and calibrated energy sum values to csv
    std::ofstream HG_ADC_sum_csv;
    HG_ADC_sum_csv.open(Form("../tmp/HL_Correlation/HG_ADC_sum_run0%d.csv", run_number));
    std::string header = "HG_ADC_sum\n";
    HG_ADC_sum_csv << header;
    for (auto i=0; i<original_event_sum.size(); i++){
        HG_ADC_sum_csv << original_event_sum[i] << "\n";
    }
    HG_ADC_sum_csv.close();

    std::ofstream Mixed_ADC_sum_csv;
    Mixed_ADC_sum_csv.open(Form("../tmp/HL_Correlation/Mixed_ADC_sum_run0%d.csv", run_number));
    header = "Mixed_ADC_sum\n";
    Mixed_ADC_sum_csv << header;
    for (auto i=0; i<calibrated_event_sum.size(); i++){
        Mixed_ADC_sum_csv << calibrated_event_sum[i] << "\n";
    }
    Mixed_ADC_sum_csv.close();

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