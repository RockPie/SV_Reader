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

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    int run_number = 64;
    bool save_to_rootfile = true;
    bool save_detail_to_png = false;
    bool save_comparison_to_png = true;
    auto bcid_cycle     = uint8_t(25);
    auto tdc_slope      = uint8_t(60);
    Double_t reconstructed_threshold_time_ns = 10000;
    Double_t hist_bin_number = 1000;
    Double_t hist_bin_min = 0;
    Double_t hist_bin_max = 22000;

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
    std::string filename_mapping_csv = "../data/config/Mapping_tb2023Sep_VMM2.csv";
    auto hglg_file_name = Form("../tmp/HL_Correlation/HL_Corr_Run0%dv.root", run_number);
    // auto hglg_file_name = "../tmp/HL_Correlation/HL_Corr_Run037v.root";

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

    std::vector<Double_t> original_event_sum;
    std::vector<Double_t> calibrated_event_sum;
    std::vector<std::pair<Double_t, Double_t>> original_event_CoM;
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

        
        auto _calib_sum = eventbuilder.get_saturation_calib_sum(mapped_event, hglg_cell_id, hglg_corr_slope_vec, hglg_corr_offset_vec, 900);

        if (_calib_sum >= 0){
            auto _event_CoM = eventbuilder.get_event_hg_CoM(event);
            original_event_CoM.push_back(_event_CoM);
        }
        original_event_sum.push_back(event_adc_sum_hg);
        calibrated_event_sum.push_back(_calib_sum);
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

    // * --- Plot channel Saturation Histogram ---
    auto parsed_frame_number = eventbuilder.get_parsed_event_number();
    Double_t saturation_threshold = 800;
    std::vector<Int_t> saturated_channel_list_hg;
    std::vector<Int_t> saturated_channel_list_lg;
    for (auto i=0; i<parsed_frame_number; i++){
        if (eventbuilder.frame_at(i)->adc > saturation_threshold){
            if (eventbuilder.is_frame_HG(*eventbuilder.frame_at(i))){
                saturated_channel_list_hg.push_back(eventbuilder.frame_at(i)->uni_channel);
            } else {
                saturated_channel_list_lg.push_back(eventbuilder.frame_at(i)->uni_channel);
                LOG(INFO) << "Saturated channel: " << eventbuilder.frame_at(i)->uni_channel << " LG";
            }
        }
    }

    auto canvas_hg_saturation = new TCanvas("HG Saturation", "HG Saturation", 800, 600);
    auto canvas_lg_saturation = new TCanvas("LG Saturation", "LG Saturation", 800, 600);

    auto hist_hg_saturation = new TH1D("HG Saturation", "HG Saturation", 64*16, 0, 64*16);
    auto hist_lg_saturation = new TH1D("LG Saturation", "LG Saturation", 64*16, 0, 64*16);

    for (auto i=0; i<saturated_channel_list_hg.size(); i++){
        hist_hg_saturation->Fill(saturated_channel_list_hg[i]);
    }

    for (auto i=0; i<saturated_channel_list_lg.size(); i++){
        hist_lg_saturation->Fill(saturated_channel_list_lg[i]);
    }

    hist_hg_saturation->GetXaxis()->SetTitle("Channel ID");
    hist_hg_saturation->GetYaxis()->SetTitle("Counts");

    hist_lg_saturation->GetXaxis()->SetTitle("Channel ID");
    hist_lg_saturation->GetYaxis()->SetTitle("Counts");

    hist_hg_saturation->SetStats(0);
    hist_lg_saturation->SetStats(0);

    canvas_hg_saturation->cd();
    hist_hg_saturation->Draw();

    canvas_lg_saturation->cd();
    hist_lg_saturation->Draw();

    if (save_comparison_to_png){
        canvas_hg_saturation->SaveAs(Form("../pics/EnergyRecon/HG_Saturation_run%d.png", run_number));
        canvas_lg_saturation->SaveAs(Form("../pics/EnergyRecon/LG_Saturation_run%d.png", run_number));
    }

    canvas_hg_saturation->Close();
    canvas_lg_saturation->Close();

    // * --- Plot event CoM distribution ---
    auto event_CoM_canvas = new TCanvas("Event CoM", "Event CoM", 1250, 1200);
    TH2D* event_CoM_hist = new TH2D("Event CoM", "Event CoM", 140, 35, 70, 140, 35, 70);

    for (auto i=0; i<original_event_CoM.size(); i++){
        event_CoM_hist->Fill(original_event_CoM[i].first, original_event_CoM[i].second);
    }

    event_CoM_hist->GetXaxis()->SetTitle("X");
    event_CoM_hist->GetYaxis()->SetTitle("Y");

    event_CoM_hist->SetTitle("");
    event_CoM_hist->SetStats(0);

    // show the color bar
    gStyle->SetOptStat(0);
    gStyle->SetPalette(1);
    gStyle->SetNumberContours(999);
    gPad->SetRightMargin(0.15);

    event_CoM_hist->Draw("colz");

    // draw detector grid
    Width_t line_width = 3;
    Color_t line_color = kGray;
    Style_t line_style = 2;

    for (int i=0; i < 35; i+=5){
        TLine* hline = new TLine(35, i+35, 70, i+35);
        hline->SetLineColor(line_color);
        hline->SetLineWidth(line_width);
        hline->SetLineStyle(line_style);
        hline->Draw();
        TLine* vline = new TLine(i+35, 35, i+35, 70);
        vline->SetLineColor(line_color);
        vline->SetLineWidth(line_width);
        vline->SetLineStyle(line_style);
        vline->Draw();
    }

    TLatex *latexInfo2 = new TLatex();
    latexInfo2->SetTextSize(0.03);
    latexInfo2->SetTextAlign(13);  //align at top
    latexInfo2->DrawLatexNDC(0.12, 0.88, Form("Event CoM Distribution"));
    latexInfo2->DrawLatexNDC(0.12, 0.84, Form("Run 0%d", run_number));
    latexInfo2->DrawLatexNDC(0.12, 0.80, Form("FoCal-H Prototype 2"));
    latexInfo2->DrawLatexNDC(0.12, 0.76, Form("September 2023 Test Beam"));

    TLatex *watermark = new TLatex();
    watermark->SetTextSize(0.12);
    watermark->SetTextAlign(13);  //align at top
    watermark->SetTextColorAlpha(kGray+2, 0.1);
    watermark->SetNDC();
    watermark->SetTextAngle(40);
    watermark->DrawLatex(0.15, 0.25, "Very Preliminary");

    for (int i=0; i < 35; i+=7){
        TLine* hline1 = new TLine(0, i+35, 35, i+35);
        hline1->SetLineColor(line_color);
        hline1->SetLineWidth(line_width);
        hline1->SetLineStyle(line_style);
        hline1->Draw();
        
        TLine* vline1 = new TLine(i+35, 0, i+35, 35);
        vline1->SetLineColor(line_color);
        vline1->SetLineWidth(line_width);
        vline1->SetLineStyle(line_style);
        vline1->Draw();

        TLine* hline2 = new TLine(70, i+35, 105, i+35);
        hline2->SetLineColor(line_color);
        hline2->SetLineWidth(line_width);
        hline2->SetLineStyle(line_style);
        hline2->Draw();

        TLine* vline2 = new TLine(i+35, 70, i+35, 105);
        vline2->SetLineColor(line_color);
        vline2->SetLineWidth(line_width);
        vline2->SetLineStyle(line_style);
        vline2->Draw();

        TLine* hline3 = new TLine(0, i, 105, i);
        hline3->SetLineColor(line_color);
        hline3->SetLineWidth(line_width);
        hline3->SetLineStyle(line_style);
        hline3->Draw();

        TLine* vline3 = new TLine(i, 0, i, 105);
        vline3->SetLineColor(line_color);
        vline3->SetLineWidth(line_width);
        vline3->SetLineStyle(line_style);
        vline3->Draw();

        TLine* hline4 = new TLine(0, i+70, 105, i+70);
        hline4->SetLineColor(line_color);
        hline4->SetLineWidth(line_width);
        hline4->SetLineStyle(line_style);
        hline4->Draw();

        TLine* vline4 = new TLine(i+70, 0, i+70, 105);
        vline4->SetLineColor(line_color);
        vline4->SetLineWidth(line_width);
        vline4->SetLineStyle(line_style);
        vline4->Draw();
    }
    // draw vertical line
    TLine* line1 = new TLine(35, 0, 35, 105);
    line1->SetLineColor(kGray);
    line1->SetLineWidth(6);
    line1->Draw();

    TLine* line2 = new TLine(70, 0, 70, 105);
    line2->SetLineColor(kGray);
    line2->SetLineWidth(6);
    line2->Draw();

    // draw horizontal line
    TLine* line3 = new TLine(0, 35, 105, 35);
    line3->SetLineColor(kGray);
    line3->SetLineWidth(6);
    line3->Draw();

    TLine* line4 = new TLine(0, 70, 105, 70);
    line4->SetLineColor(kGray);
    line4->SetLineWidth(6);
    line4->Draw();    

    if (save_comparison_to_png){
        event_CoM_canvas->SaveAs(Form("../pics/PositionRecon/Event_CoM_run%d.png", run_number));
    }

    event_CoM_canvas->Close();
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