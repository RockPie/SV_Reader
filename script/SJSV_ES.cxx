#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "TFile.h"
#include "TTree.h"
#include "TLatex.h"
#include "TF1.h"
#include "TGraph.h"
#include "csv.h"
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    std::vector<std::string> root_file_names;
    std::vector<Int_t> beam_energies;
    std::string config_name;

    std::string common_info_line1 = "FoCal-H Prototype 2";
    std::string common_info_line2 = "SPS H4 Beam Test";
    std::string common_info_line3 = "September 2023";

    std::vector<Int_t> run_module_number;

    const std::vector<Int_t> center_module_number = {5};
    const std::vector<Int_t> corner_module_number = {1,3,7,9};
    const std::vector<Int_t> edge_module_number = {2,4,6,8};

    auto HG_n_bins = 1000;
    auto HG_min = 0;
    auto HG_max = 26000;

    auto LG_n_bins = 1000;
    auto LG_min = 0;
    auto LG_max = 3000;

    auto HG_adc_max = 0.02;
    auto LG_adc_max = 0.012;

    int config = 9;

    switch (config)
    {
    case 1: {
        config_name = "Hadron Energy Scan with 55.5 V Bias";
        root_file_names = std::vector<std::string>{
           "../tmp/analysis_rcsl_Run050v.root",
           "../tmp/analysis_rcsl_Run051v.root",
           "../tmp/analysis_rcsl_Run052v.root",
           "../tmp/analysis_rcsl_Run053v.root",
           "../tmp/analysis_rcsl_Run054v.root",
           "../tmp/analysis_rcsl_Run055v.root",
           "../tmp/analysis_rcsl_Run056v.root",
           "../tmp/analysis_rcsl_Run057v.root"};
        beam_energies = std::vector<Int_t>{
            350,
            300,
            250,
            200,
            150,
            100,
            80,
            60};
        
        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 40000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 4000;

        HG_adc_max = 0.013;
        LG_adc_max = 0.012;

        break;
    }
    case 2: {
        config_name = "Electron Energy Scan with 55.5 V Bias";
        root_file_names = std::vector<std::string>{
            "../tmp/analysis_rcsl_Run058v.root",
            "../tmp/analysis_rcsl_Run059v.root",
            "../tmp/analysis_rcsl_Run060v.root",
            "../tmp/analysis_rcsl_Run062v.root",
            "../tmp/analysis_rcsl_Run063v.root",
            "../tmp/analysis_rcsl_Run064v.root",};
        beam_energies = std::vector<Int_t>{
            200,
            150,
            100,
            80,
            60,
            20};
        
        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 17000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 3000;

        HG_adc_max = 0.014;
        LG_adc_max = 0.015;

        break;
    }
    case 3: {
        config_name = "Hadron Energy Scan with 54.5 V Bias";
        root_file_names = std::vector<std::string>{
            "../tmp/analysis_rcslr_Run066v.root",
            "../tmp/analysis_rcslr_Run067v.root",
            "../tmp/analysis_rcslr_Run068v.root",
            "../tmp/analysis_rcslr_Run069v.root",
            "../tmp/analysis_rcslr_Run070v.root",
            "../tmp/analysis_rcslr_Run071v.root",
            "../tmp/analysis_rcslr_Run072v.root",};
        beam_energies = std::vector<Int_t>{
            350,
            250,
            200,
            150,
            100,
            80,
            60};
        
        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 27000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 1800;

        HG_adc_max = 0.018;
        LG_adc_max = 0.012;

        break;
    }
    case 4: {
        config_name = "Electron Energy Scan with 54.5 V Bias";
        root_file_names = std::vector<std::string>{
            "../tmp/analysis_rcsl_Run073v.root",
            "../tmp/analysis_rcsl_Run074v.root",
            "../tmp/analysis_rcsl_Run075v.root",
            "../tmp/analysis_rcsl_Run076v.root",
            "../tmp/analysis_rcsl_Run077v.root",
            "../tmp/analysis_rcsl_Run078v.root",
            "../tmp/analysis_rcsl_Run079v.root",};
        beam_energies = std::vector<Int_t>{
            250,
            200,
            150,
            100,
            80,
            60,
            20,};
            
        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 15000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 2000;

        HG_adc_max = 0.02;
        LG_adc_max = 0.012;

        break;
    }
    case 5: {
        config_name = "Hadron Energy Scan with 56.5 V Bias";
        root_file_names = std::vector<std::string>{
            "../tmp/analysis_rcslr_Run037v.root",
            "../tmp/analysis_rcslr_Run022v.root",
            "../tmp/analysis_rcslr_Run023v.root",
            "../tmp/analysis_rcslr_Run024v.root",
            "../tmp/analysis_rcslr_Run025v.root",
            "../tmp/analysis_rcslr_Run026v.root",
            "../tmp/analysis_rcslr_Run027v.root",
            "../tmp/analysis_rcslr_Run028v.root",};

        beam_energies = std::vector<Int_t>{
            350,
            300,
            250,
            200,
            150,
            100,
            80,
            60};

        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 47000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 5000;

        HG_adc_max = 0.012;
        LG_adc_max = 0.012;

        break;
    }

    case 6: {
        config_name = "Hadron Position Scan of 350 GeV";
        root_file_names = std::vector<std::string>{
            "../tmp/analysis_rcslr_Run035v.root",
            "../tmp/analysis_rcslr_Run030v.root",
            "../tmp/analysis_rcslr_Run031v.root",
            "../tmp/analysis_rcslr_Run036v.root",
            "../tmp/analysis_rcslr_Run037v.root",
            "../tmp/analysis_rcslr_Run032v.root",
            "../tmp/analysis_rcslr_Run029v.root",
            "../tmp/analysis_rcslr_Run034v.root",
            "../tmp/analysis_rcslr_Run033v.root"};

        run_module_number = std::vector<Int_t>{
            1,2,3,4,5,6,7,8,9};

        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 50000;

        LG_n_bins = 500;
        LG_min = 0;
        LG_max = 5500;

        HG_adc_max = 0.007;
        LG_adc_max = 0.014;
        LOG(INFO) << "File names: " << root_file_names[0];
        break;
    }

    case 7: {
        config_name = "Electron Position Scan of 100 GeV";
        root_file_names = std::vector<std::string>{
            "../tmp/analysis_rcslr_Run040v.root",
            "../tmp/analysis_rcslr_Run045v.root",
            "../tmp/analysis_rcslr_Run044v.root",
            "../tmp/analysis_rcslr_Run039v.root",
            "../tmp/analysis_rcslr_Run038v.root",
            "../tmp/analysis_rcslr_Run043v.root",
            "../tmp/analysis_rcslr_Run046v.root",
            "../tmp/analysis_rcslr_Run041v.root",
            "../tmp/analysis_rcslr_Run042v.root"};

        run_module_number = std::vector<Int_t>{
            1,2,3,4,5,6,7,8,9};

        HG_n_bins = 2000;
        HG_min = 0;
        HG_max = 12000;

        LG_n_bins = 5000;
        LG_min = 0;
        LG_max = 2500;

        HG_adc_max = 0.0045;
        LG_adc_max = 0.007;
        LOG(INFO) << "File names: " << root_file_names[0];
        break;
    }
    case 8: {
        config_name = "Hadron Gain Scan of 1 mV/fC with 54.5 V Bias";
        root_file_names = std::vector<std::string>{
           "../tmp/analysis_rcslr_Run081v.root",
           "../tmp/analysis_rcslr_Run084v.root",
           "../tmp/analysis_rcslr_Run085v.root",};
        beam_energies = std::vector<Int_t>{
            350,
            250,
            60};
        
        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 50000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 4000;

        HG_adc_max = 0.014;
        LG_adc_max = 0.016;
        break;
    }

    case 9: {
        config_name = "Electron Gain Scan of 1 mV/fC with 54.5 V Bias";
        root_file_names = std::vector<std::string>{
           "../tmp/analysis_rcslr_Run086v.root",
           "../tmp/analysis_rcslr_Run087v.root",};
        beam_energies = std::vector<Int_t>{
            250,
            20};
        
        HG_n_bins = 1000;
        HG_min = 0;
        HG_max = 25000;

        LG_n_bins = 1000;
        LG_min = 0;
        LG_max = 4000;

        HG_adc_max = 0.023;
        LG_adc_max = 0.035;
        break;

    }

    default:
        break;
    }

    std::vector<Int_t> color_list;
    Int_t _center_cnt = 0;
    Int_t _corner_cnt = 0;
    Int_t _edge_cnt = 0;
    if (run_module_number.size() > 0){
        for (auto module_number : run_module_number){
            if (std::find(center_module_number.begin(),     center_module_number.end(), module_number) !=   center_module_number.end()){
                color_list.push_back(kRed);
                _center_cnt++;
            }
            else if (std::find(corner_module_number.begin(),    corner_module_number.end(), module_number) !=  corner_module_number.end()){
                color_list.push_back(kOrange + 1 +  _corner_cnt*2);
                _corner_cnt++;
            }
            else if (std::find(edge_module_number.begin(),  edge_module_number.end(), module_number) !=  edge_module_number.end()){
                color_list.push_back(kGreen + 1 + _edge_cnt);
                _edge_cnt++;
            }
            else{
                color_list.push_back(kBlack);
            }
        }
    }

    std::vector<std::vector<Double_t>*> HG_adc_list;
    std::vector<std::vector<Double_t>*> LG_adc_list;
    std::vector<Double_t> HG_fit_mu;
    std::vector<Double_t> HG_fit_sigma;
    std::vector<Double_t> LG_fit_mu = {2250, 1700, 1500, 1000, 500, 150, 70, 0};

    // for (auto i=0; i < beam_energies.size(); i++){
    //     LG_fit_mu[i] *= 18;
    // }
    std::vector<Double_t> LG_fit_sigma;
    std::vector<Double_t> HG_fit_resolution;


    for (auto i=0; i < root_file_names.size(); i++){
        auto root_file_name = root_file_names[i].c_str();
        auto beam_energy = beam_energies[i];

        TFile* root_file = new TFile(root_file_name, "READ");
        root_file->cd("HG");
        TTree* HGtree = (TTree*)gDirectory->Get("event_HG_adc");
        root_file->cd("LG");
        TTree* LGtree = (TTree*)gDirectory->Get("event_LG_adc");

        std::vector<Double_t>* HG_adc = 0;
        std::vector<Double_t>* LG_adc = 0;

        HGtree->SetBranchAddress("event_HG_adc", &HG_adc);
        LGtree->SetBranchAddress("event_LG_adc", &LG_adc);

        auto n_entries = HGtree->GetEntries();
        for (auto i=0; i < n_entries; i++){
            HGtree->GetEntry(i);
            LGtree->GetEntry(i);
            LOG(INFO) << "HG_adc size: " << HG_adc->size();
            LOG(INFO) << "LG_adc size: " << LG_adc->size();
        }

        root_file->Close();

        HG_adc_list.push_back(HG_adc);
        LG_adc_list.push_back(LG_adc);
    }

    LOG(INFO) << "HG_adc_list size: " << HG_adc_list.size();

    std::vector<TH1D*> HG_adc_hist_list;
    std::vector<TH1D*> LG_adc_hist_list;

    // top right legend
    TLegend* HG_energy_legend = new TLegend(0.75, 0.5, 0.9, 0.9);
    TLegend* LG_energy_legend = new TLegend(0.75, 0.5, 0.9, 0.9);
    HG_energy_legend->SetTextFont(62);
    LG_energy_legend->SetTextFont(62);
    // HG_energy_legend->SetTextSize(0.03);
    // LG_energy_legend->SetTextSize(0.03);
    HG_energy_legend->SetTextAlign(32);
    LG_energy_legend->SetTextAlign(32);

    TLatex* water_mark = new TLatex(0.2, 0.2, "Very Preliminary");
    water_mark->SetNDC();
    water_mark->SetTextColorAlpha(kGray+2, 0.1);
    water_mark->SetTextAngle(30);
    water_mark->SetTextSize(0.15);

        // add text to the plot
    TLatex* text_line0 = new TLatex(0.75, 0.85, config_name.c_str());
    TLatex* text_line1 = new TLatex(0.75, 0.8, common_info_line1.c_str());
    TLatex* text_line2 = new TLatex(0.75, 0.75, common_info_line2.c_str());
    TLatex* text_line3 = new TLatex(0.75, 0.7, common_info_line3.c_str());

    // right align
    text_line0->SetTextAlign(31);
    text_line1->SetTextAlign(31);
    text_line2->SetTextAlign(31);
    text_line3->SetTextAlign(31);

    // set font
    text_line0->SetTextFont(62);
    text_line1->SetTextFont(62);
    text_line2->SetTextFont(62);
    text_line3->SetTextFont(62);

    text_line0->SetNDC();
    text_line1->SetNDC();
    text_line2->SetNDC();
    text_line3->SetNDC();

    text_line0->SetTextSize(0.03);
    text_line1->SetTextSize(0.03);
    text_line2->SetTextSize(0.03);
    text_line3->SetTextSize(0.03);
    
    auto _global_max_HG = 0.0;
    auto _global_max_LG = 0.0;

    for (auto i=0; i < HG_adc_list.size(); i++){
        auto HG_adc = HG_adc_list[i];
        auto LG_adc = LG_adc_list[i];

        auto HG_adc_hist = new TH1D(Form("HG_adc_hist_%d", i), Form("HG_adc_hist_%d", i), HG_n_bins, HG_min, HG_max);
        auto LG_adc_hist = new TH1D(Form("LG_adc_hist_%d", i), Form("LG_adc_hist_%d", i), LG_n_bins, LG_min, LG_max);

        // set title size
        HG_adc_hist->SetTitleSize(0.03);
        LG_adc_hist->SetTitleSize(0.03);

        // set title offset
        HG_adc_hist->SetTitleOffset(1.5);
        LG_adc_hist->SetTitleOffset(1.5);

        HG_adc_hist->SetTitle("High Gain ADC Distribution");
        LG_adc_hist->SetTitle("Low Gain ADC Distribution");

        HG_adc_hist->SetStats(0);
        LG_adc_hist->SetStats(0);

        HG_adc_hist->GetXaxis()->SetTitle("ADC Sum");
        LG_adc_hist->GetXaxis()->SetTitle("ADC Sum");

        HG_adc_hist->GetYaxis()->SetTitle("Normalized Counts");
        LG_adc_hist->GetYaxis()->SetTitle("Normalized Counts");

        std::string HG_energy_legend_text;
        std::string LG_energy_legend_text;

        if (beam_energies.size() > 0){
            HG_energy_legend_text = Form("%d GeV", beam_energies[i]);
            LG_energy_legend_text = Form("%d GeV", beam_energies[i]);
        } else {
            HG_energy_legend_text = Form("Module %d", run_module_number[i]);
            LG_energy_legend_text = Form("Module %d", run_module_number[i]);
        }
        

        // add run info
        HG_energy_legend_text += " (" + root_file_names[i].substr(25, 3) + ")";
        LG_energy_legend_text += " (" + root_file_names[i].substr(25, 3) + ")";

        HG_energy_legend->AddEntry(HG_adc_hist, HG_energy_legend_text.c_str(), "l");
        LG_energy_legend->AddEntry(LG_adc_hist, LG_energy_legend_text.c_str(), "l");

        // move to right to show y axis
        HG_adc_hist->GetYaxis()->SetTitleOffset(1.5);
        LG_adc_hist->GetYaxis()->SetTitleOffset(1.5);


        for (auto j=0; j < HG_adc->size(); j++){
            HG_adc_hist->Fill(HG_adc->at(j));
            LG_adc_hist->Fill(LG_adc->at(j));
        }

        // normalize
        auto HG_integral = HG_adc_hist->Integral();
        auto LG_integral = LG_adc_hist->Integral();

        HG_adc_hist->Scale(1.0/HG_integral);
        LG_adc_hist->Scale(1.0/LG_integral);

        auto _max_HG = HG_adc_hist->GetMaximum();
        auto _max_LG = LG_adc_hist->GetMaximum();

        if (_max_HG > _global_max_HG)
            _global_max_HG = _max_HG;

        if (_max_LG > _global_max_LG)
            _global_max_LG = _max_LG;

        // fitting with gaussian
        auto HG_mean = HG_adc_hist->GetMean();
        auto HG_sigma = HG_adc_hist->GetRMS();
        auto LG_mean = LG_adc_hist->GetMean();
        auto LG_sigma = LG_adc_hist->GetRMS();

        Double_t fit_area_offset = 0;
        Double_t sigma_multiplier = 2;

        TF1* HG_gaus = new TF1("HG_gaus", "gaus", HG_mean + fit_area_offset - sigma_multiplier*HG_sigma, HG_mean + fit_area_offset +    sigma_multiplier*HG_sigma);
        TF1* LG_gaus = new TF1("LG_gaus", "gaus", LG_mean + fit_area_offset - sigma_multiplier*LG_sigma, LG_mean + fit_area_offset + sigma_multiplier*LG_sigma);

        HG_gaus->SetLineColor(kGreen);
        LG_gaus->SetLineColor(kGreen);
        HG_gaus->SetLineWidth(2);
        LG_gaus->SetLineWidth(2);
        HG_gaus->SetLineStyle(2);
        LG_gaus->SetLineStyle(2);

        // HG_adc_hist->Fit("HG_gaus", "R", "", HG_mean + fit_area_offset - sigma_multiplier*HG_sigma, HG_mean + fit_area_offset +    sigma_multiplier*HG_sigma);
        // LG_adc_hist->Fit("LG_gaus", "R");

        HG_fit_mu.push_back(HG_gaus->GetParameter(1));
        HG_fit_sigma.push_back(HG_gaus->GetParameter(2));
        HG_fit_resolution.push_back(HG_gaus->GetParameter(2) / HG_gaus->GetParameter(1));
        // LG_fit_mu.push_back(LG_gaus->GetParameter(1) * 18);
        // LG_fit_sigma.push_back(LG_gaus->GetParameter(2) * 18);

        HG_adc_hist_list.push_back(HG_adc_hist);
        LG_adc_hist_list.push_back(LG_adc_hist);
    }

    for (auto _hist: HG_adc_hist_list){
        _hist->GetYaxis()->SetRangeUser(0, HG_adc_max);
    }

    for (auto _hist: LG_adc_hist_list){
        _hist->GetYaxis()->SetRangeUser(0, LG_adc_max);
    }

    auto ch = new TCanvas("ch", "ch", 1200, 800);
    ch->SetTitle(config_name.c_str());
    for (auto _hist_index=0; _hist_index < HG_adc_hist_list.size(); _hist_index++){
        auto _hist = HG_adc_hist_list[_hist_index];
        if (run_module_number.size() > 0){
            _hist->SetLineColor(color_list[_hist_index]);
        }
        else {
            _hist->SetLineColor(_hist_index+1);
        }
        _hist->SetLineWidth(3);
        //_hist->SetMarkerSize(5);
        if (_hist_index == 0){
            _hist->Draw("l");
        }else{
            _hist->Draw("l same");
        }
    }
    // set dashed grid line
    ch->SetGrid();

    text_line0->Draw();
    text_line1->Draw();
    text_line2->Draw();
    text_line3->Draw();

    HG_energy_legend->Draw();
    water_mark->Draw();

    ch->SaveAs(Form("../pics/HG_adc_%d.png", config));
    ch->Close();


    auto cl = new TCanvas("cl", "cl", 1200, 800);
    cl->SetTitle(config_name.c_str());
    //cl->SetLogy();
    for (auto _hist_index=0; _hist_index < LG_adc_hist_list.size(); _hist_index++){
        auto _hist = LG_adc_hist_list[_hist_index];
        if (run_module_number.size() > 0){
            _hist->SetLineColor(color_list[_hist_index]);
        }
        else {
            _hist->SetLineColor(_hist_index+1);
        }
        _hist->SetLineWidth(4);
        _hist->SetMarkerSize(5);
        if (_hist_index == 0){
            _hist->Draw();
        }else{
        _hist->Draw("same");
        }
    }
    cl->SetGrid();

    text_line0->Draw();
    text_line1->Draw();
    text_line2->Draw();
    text_line3->Draw();
    LG_energy_legend->Draw();
    water_mark->Draw();
    cl->SaveAs(Form("../pics/LG_adc_%d.png", config));
    cl->Close();

    // print linearity plot
    auto clinear = new TCanvas("linearity", "linearity", 1200, 800);
    clinear->SetTitle(config_name.c_str());
    std::vector<Double_t> HG_linearity_x;
    std::vector<Double_t> LG_linearity_x;
    for (auto i=0; i < beam_energies.size(); i++){
        HG_linearity_x.push_back(Double_t(beam_energies[i]));
        LG_linearity_x.push_back(Double_t(beam_energies[i]));
    }
    auto HG_linearity = new TGraph(HG_linearity_x.size(), &HG_linearity_x[0], &HG_fit_mu[0]);
    auto LG_linearity = new TGraph(LG_linearity_x.size(), &LG_linearity_x[0], &LG_fit_mu[0]);

    HG_linearity->SetMarkerStyle(20);
    HG_linearity->SetMarkerSize(2);
    HG_linearity->SetMarkerColor(kBlue);
    HG_linearity->SetLineColor(kBlue);
    HG_linearity->SetLineWidth(2);
    HG_linearity->SetTitle("Linearity");

    LG_linearity->SetMarkerStyle(20);
    LG_linearity->SetMarkerSize(2);
    LG_linearity->SetMarkerColor(kRed);
    LG_linearity->SetLineColor(kRed);
    LG_linearity->SetLineWidth(2);
    LG_linearity->SetTitle("Linearity");

    HG_linearity->GetXaxis()->SetTitle("Beam Energy [GeV]");
    HG_linearity->GetYaxis()->SetTitle("ADC [ADC]");
    HG_linearity->GetYaxis()->SetRangeUser(0, 40000);
    HG_linearity->GetXaxis()->SetRangeUser(0, 400);

    LG_linearity->GetXaxis()->SetTitle("Beam Energy [GeV]");
    LG_linearity->GetYaxis()->SetTitle("ADC [ADC]");
    LG_linearity->GetYaxis()->SetRangeUser(0, 40000);
    LG_linearity->GetXaxis()->SetRangeUser(0, 400);

    HG_linearity->Draw("APL");
    LG_linearity->Draw("PL same");

    // legend to top left
    TLegend* linearity_legend = new TLegend(0.1, 0.7, 0.25, 0.9);
    linearity_legend->SetTextFont(62);
    linearity_legend->SetTextSize(0.03);
    linearity_legend->SetTextAlign(32);
    linearity_legend->AddEntry(HG_linearity, "High Gain", "p");
    linearity_legend->AddEntry(LG_linearity, "Low Gain", "p");
    linearity_legend->Draw();

    clinear->SetGrid();
    //clinear->SaveAs(Form("../pics/linearity_%d.png", config));
    clinear->Close();

    auto cresolution = new TCanvas("resolution", "resolution", 1400, 800);
    cresolution->SetTitle(config_name.c_str());
    std::vector<Double_t> _beam_energies_double;
    for (auto i=0; i < beam_energies.size(); i++){
        _beam_energies_double.push_back(Double_t(beam_energies[i]));
    }
    auto resolution_graph = new TGraph(HG_fit_resolution.size(), &_beam_energies_double[0], &HG_fit_resolution[0]);
    resolution_graph->SetMarkerStyle(20);
    resolution_graph->SetMarkerSize(2);
    resolution_graph->SetMarkerColor(kBlue);
    resolution_graph->SetLineColor(kBlue);
    resolution_graph->SetTitle("");

    resolution_graph->GetXaxis()->SetRangeUser(0, 400);
    resolution_graph->GetYaxis()->SetRangeUser(0.1, 0.4);
    resolution_graph->GetXaxis()->SetLimits(0, 400);
    resolution_graph->GetYaxis()->SetLimits(0.1, 0.4);

    resolution_graph->GetXaxis()->SetTitle("ADC [ADC]");
    resolution_graph->GetYaxis()->SetTitle("Resolution");

    resolution_graph->Draw("AP");

    cresolution->SetGrid();
    cresolution->SaveAs(Form("../pics/PSresolution_%d.png", config));
    cresolution->Close();

    // save HG mu to csv file
    std::ofstream HG_mu_csv;
    auto header = "Beam,ADC,Sigma,Resolution";
    HG_mu_csv.open(Form("../tmp/PS_mu_%d.csv", config));
    HG_mu_csv << header << std::endl;
    for (auto i=0; i < beam_energies.size(); i++){
        HG_mu_csv << beam_energies[i] << "," << HG_fit_mu[i] << "," << HG_fit_sigma[i] << "," << HG_fit_resolution[i] << std::endl;
    }
    HG_mu_csv.close();


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
