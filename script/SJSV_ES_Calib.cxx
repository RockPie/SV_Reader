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

    std::vector<std::string> csv_file_names;
    std::vector<Int_t> beam_energies;
    std::string config_name;

    std::vector<Double_t> fit_area_offset_list = {
        -1500, -1000, -500, 0, 500, 1000, 1500
    };
    std::vector<Double_t> sigma_multiplier_list = {
        1, 2, 3
    };

    std::string common_info_line1 = "FoCal-H Prototype 2";
    std::string common_info_line2 = "SPS H4 Beam Test";
    std::string common_info_line3 = "September 2023";

    auto Mixed_n_bins = 1000;
    auto Mixed_min = 0;
    auto Mixed_max = 26000;

    auto Mixed_adc_max = 0.02;
    int config = 3;

    switch (config)
    {
    case 1: {
        config_name = "Hadron Energy Scan with 56.5 V Bias";
        csv_file_names = std::vector<std::string>{
            "../tmp/HL_Correlation/Mixed_ADC_sum_run037.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run022.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run023.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run024.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run025.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run026.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run027.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run028.csv",
        };

        beam_energies = std::vector<Int_t>{
            350,
            300,
            250,
            200,
            150,
            100,
            80,
            60};
        
        Mixed_n_bins = 1000;
        Mixed_min = 0;
        Mixed_max = 60000;

        Mixed_adc_max = 0.013;

        break;
    }
    case 2:{
        config_name = "Hadron Energy Scan with 54.5 V Bias";
        csv_file_names = std::vector<std::string>{
            "../tmp/HL_Correlation/Mixed_ADC_sum_run066.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run067.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run068.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run069.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run070.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run071.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run072.csv",
        };

        beam_energies = std::vector<Int_t>{
            350,
            250,
            200,
            150,
            100,
            80,
            60};
        
        Mixed_n_bins = 1000;
        Mixed_min = 0;
        Mixed_max = 30000;

        Mixed_adc_max = 0.015;

        break;
    }
    case 3:{
        config_name = "Hadron Energy Scan with 55.5 V Bias";
        csv_file_names = std::vector<std::string>{
           "../tmp/HL_Correlation/Mixed_ADC_sum_run050.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run051.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run052.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run053.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run054.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run055.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run056.csv",
           "../tmp/HL_Correlation/Mixed_ADC_sum_run057.csv"};
        beam_energies = std::vector<Int_t>{
            350,
            300,
            250,
            200,
            150,
            100,
            80,
            60};
        
        Mixed_n_bins = 1000;
        Mixed_min = 0;
        Mixed_max = 45000;

        Mixed_adc_max = 0.0135;
        break;
    }
    case 4:{
        config_name = "Electron Energy Scan with 55.5 V Bias";
        csv_file_names = std::vector<std::string>{
            "../tmp/HL_Correlation/Mixed_ADC_sum_run058.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run059.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run060.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run062.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run063.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run064.csv",};
        beam_energies = std::vector<Int_t>{
            200,
            150,
            100,
            80,
            60,
            20};
        
        Mixed_n_bins = 1000;
        Mixed_min = 0;
        Mixed_max = 34000;

        Mixed_adc_max = 0.018;

        break;
    }
    case 5: {
        config_name = "Electron Energy Scan with 54.5 V Bias";
        csv_file_names = std::vector<std::string>{
            "../tmp/HL_Correlation/Mixed_ADC_sum_run073.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run074.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run075.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run076.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run077.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run078.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run079.csv",};
        beam_energies = std::vector<Int_t>{
            250,
            200,
            150,
            100,
            80,
            60,
            20,};
            
        Mixed_n_bins = 1000;
        Mixed_min = 0;
        Mixed_max = 22000;
        Mixed_adc_max = 0.02;

        break;
    }

    default:
        break;
    }


    std::vector<std::vector<Double_t>*> Mixed_adc_list;
    std::vector<Double_t> Mixed_fit_mu;
    std::vector<Double_t> Mixed_fit_sigma;
    std::vector<Double_t> Mixed_fit_resolution;
    std::vector<Double_t> Mixed_fit_para_range;
    std::vector<Double_t> Mixed_fit_para_offset;
    std::vector<Double_t> Mixed_fit_para_adc;

    for (auto i=0; i < csv_file_names.size(); i++){
        auto csv_file_name = csv_file_names[i];
        auto beam_energy = beam_energies[i];
        LOG(INFO) << "Processing " << csv_file_name << " ...";

        io::CSVReader<1> _in(csv_file_name);
        Double_t Mixed_ADC_sum;
        auto Mixed_ADC_sum_v = new std::vector<Double_t>;
        // skip header
        _in.read_header(io::ignore_extra_column, "Mixed_ADC_sum");
        while(_in.read_row(Mixed_ADC_sum)){
            Mixed_ADC_sum_v->push_back(Mixed_ADC_sum);
        }
        LOG(DEBUG) << "Mixed_ADC_sum_v size: " << Mixed_ADC_sum_v->size();
        Mixed_adc_list.push_back(Mixed_ADC_sum_v);
        
    }
    LOG(DEBUG) << "Mixed_adc_list size: " << Mixed_adc_list.size();
    std::vector<TH1D*> Mixed_adc_hist_list;

    // top right legend
    TLegend* Mixed_energy_legend = new TLegend(0.735, 0.5, 0.9, 0.9);
    Mixed_energy_legend->SetTextFont(62);
    Mixed_energy_legend->SetTextSize(0.03);
    Mixed_energy_legend->SetTextAlign(32);

    TLatex* water_mark = new TLatex(0.2, 0.2, "Very Preliminary");
    water_mark->SetNDC();
    water_mark->SetTextColorAlpha(kGray+2, 0.1);
    water_mark->SetTextAngle(30);
    water_mark->SetTextSize(0.15);

        // add text to the plot
    TLatex* text_line0 = new TLatex(0.73, 0.85, config_name.c_str());
    TLatex* text_line1 = new TLatex(0.73, 0.8, common_info_line1.c_str());
    TLatex* text_line2 = new TLatex(0.73, 0.75, common_info_line2.c_str());
    TLatex* text_line3 = new TLatex(0.73, 0.7, common_info_line3.c_str());

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
    
    auto _global_max_Mixed = 0.0;

 

    for (auto _fit_offset: fit_area_offset_list){
        for (auto _fit_sigma: sigma_multiplier_list) {

            for (auto i=0; i < Mixed_adc_list.size(); i++){
                auto Mixed_adc = Mixed_adc_list[i];

                auto Mixed_adc_hist = new TH1D(Form("Mixed_adc_hist_%d", i),        Form("Mixed_adc_hist_%d", i), Mixed_n_bins, Mixed_min,      Mixed_max);

                // set title size
                Mixed_adc_hist->SetTitleSize(0.03);
                Mixed_adc_hist->SetTitleOffset(1.5);
                Mixed_adc_hist->SetTitle("Mixed ADC Distribution");
                Mixed_adc_hist->SetStats(0);
                Mixed_adc_hist->GetXaxis()->SetTitle("ADC");
                Mixed_adc_hist->GetYaxis()->SetTitle("Normalized Counts");

                std::string Mixed_energy_legend_text = Form("%d GeV",       beam_energies[i]);

                // add run info
                Mixed_energy_legend_text += " (" + csv_file_names[i].substr     (39, 3) + ")";
                Mixed_energy_legend->AddEntry(Mixed_adc_hist,       Mixed_energy_legend_text.c_str(), "l");

                // move to right to show y axis
                Mixed_adc_hist->GetYaxis()->SetTitleOffset(1.5);

                LOG(DEBUG) << "Mixed_adc->size(): " << Mixed_adc->size();
                for (auto j=0; j < Mixed_adc->size(); j++){
                    Mixed_adc_hist->Fill(Mixed_adc->at(j));
                   // LOG(DEBUG) << "Mixed_adc->at(j): " << Mixed_adc->at(j);
                }

                // normalize
                auto Mixed_integral = Mixed_adc_hist->Integral();

                Mixed_adc_hist->Scale(1.0/Mixed_integral);

                auto _max_Mixed = Mixed_adc_hist->GetMaximum();

                if (_max_Mixed > _global_max_Mixed)
                    _global_max_Mixed = _max_Mixed;

                // fitting with gaussian
                auto Mixed_mean = Mixed_adc_hist->GetMean();
                auto Mixed_sigma = Mixed_adc_hist->GetRMS();
                auto Mixed_max = Mixed_adc_hist->GetMaximum();

                // TF1* Mixed_gaus = new TF1("Mixed_gaus", "gaus",      Mixed_mean + fit_area_offset - sigma_multiplier*Mixed_sigma,        Mixed_mean + fit_area_offset +         sigma_multiplier*Mixed_sigma);
                TF1* CrystalBall = new TF1("CrystalBall", "crystalball");

                // set initial parameters
                CrystalBall->SetParameter(0, Mixed_max);
                CrystalBall->SetParameter(1, Mixed_mean);
                CrystalBall->SetParameter(2, Mixed_sigma);
                CrystalBall->SetParameter(3, 1.5);
                CrystalBall->SetParameter(4, 0.5);

                // Mixed_gaus->SetLineColor(kGreen);
                // Mixed_gaus->SetLineWidth(3);
                // Mixed_gaus->SetLineStyle(2);

                CrystalBall->SetLineColor(kRed);
                CrystalBall->SetLineWidth(3);
                CrystalBall->SetLineStyle(2);

                // Mixed_adc_hist->Fit("Mixed_gaus", "R", "", Mixed_mean +      fit_area_offset - sigma_multiplier*Mixed_sigma, Mixed_mean +        fit_area_offset +    sigma_multiplier*Mixed_sigma);
                auto fit_range_min = Mixed_mean + _fit_offset - _fit_sigma * Mixed_sigma;
                if (fit_range_min < 0)
                    fit_range_min = 0;
                auto fit_range_max = Mixed_mean + _fit_offset + _fit_sigma * Mixed_sigma;
                if (fit_range_max > 45000)
                    fit_range_max = 45000;
                Mixed_adc_hist->Fit("CrystalBall", "R", "", fit_range_min,      fit_range_max);

                // Mixed_fit_mu.push_back(Mixed_gaus->GetParameter(1));
                // Mixed_fit_sigma.push_back(Mixed_gaus->GetParameter(2));
                // Mixed_fit_resolution.push_back(Mixed_gaus->GetParameter      (2) / Mixed_gaus->GetParameter(1));
                // LG_fit_mu.push_back(LG_gaus->GetParameter(1) * 18);
                // LG_fit_sigma.push_back(LG_gaus->GetParameter(2) * 18);

                Mixed_fit_mu.push_back(CrystalBall->GetParameter(1));
                Mixed_fit_sigma.push_back(CrystalBall->GetParameter(2));
                Mixed_fit_resolution.push_back(CrystalBall->GetParameter        (2) / CrystalBall->GetParameter(1));
                Mixed_fit_para_offset.push_back(_fit_offset);
                Mixed_fit_para_range.push_back(_fit_sigma);
                Mixed_fit_para_adc.push_back(beam_energies[i]);

                Mixed_adc_hist_list.push_back(Mixed_adc_hist);
            }
        }
    }



    for (auto _hist: Mixed_adc_hist_list){
        _hist->GetYaxis()->SetRangeUser(0, Mixed_adc_max);
    }

    auto ch = new TCanvas("ch", "ch", 1200, 800);
    ch->SetTitle(config_name.c_str());
    for (auto _hist_index=0; _hist_index < Mixed_adc_hist_list.size(); _hist_index++){
        auto _hist = Mixed_adc_hist_list[_hist_index];
        _hist->SetLineColor(_hist_index+1);
        _hist->SetLineWidth(5);
        _hist->SetMarkerSize(5);
        if (_hist_index == 0){
            _hist->Draw();
        }else{
            _hist->Draw("same");
        }
    }
    // set dashed grid line
    ch->SetGrid();

    text_line0->Draw();
    text_line1->Draw();
    text_line2->Draw();
    text_line3->Draw();

    Mixed_energy_legend->Draw();
    water_mark->Draw();

    ch->SaveAs(Form("../pics/Mixed_adc_%d.png", config));
    ch->Close();

    // print linearity plot
    auto clinear = new TCanvas("linearity", "linearity", 1200, 800);
    clinear->SetTitle(config_name.c_str());
    std::vector<Double_t> Mixed_linearity_x;
    for (auto i=0; i < beam_energies.size(); i++){
        Mixed_linearity_x.push_back(Double_t(beam_energies[i]));
    }
    auto Mixed_linearity = new TGraph(Mixed_linearity_x.size(), &Mixed_linearity_x[0], &Mixed_fit_mu[0]);
    Mixed_linearity->SetMarkerStyle(20);
    Mixed_linearity->SetMarkerSize(2);
    Mixed_linearity->SetMarkerColor(kBlue);
    Mixed_linearity->SetLineColor(kBlue);
    Mixed_linearity->SetLineWidth(2);
    Mixed_linearity->SetTitle("Linearity");

    Mixed_linearity->GetXaxis()->SetTitle("Beam Energy [GeV]");
    Mixed_linearity->GetYaxis()->SetTitle("ADC [ADC]");
    Mixed_linearity->GetYaxis()->SetRangeUser(0, Mixed_max);
    Mixed_linearity->GetXaxis()->SetRangeUser(0, 400);

    Mixed_linearity->Draw("APL");

    // legend to top left
    TLegend* linearity_legend = new TLegend(0.1, 0.7, 0.25, 0.9);
    linearity_legend->SetTextFont(62);
    linearity_legend->SetTextSize(0.03);
    linearity_legend->SetTextAlign(32);
    linearity_legend->AddEntry(Mixed_linearity, "Mixed", "p");
    linearity_legend->Draw();

    clinear->SetGrid();
    clinear->SaveAs(Form("../pics/mixed_linearity_%d.png", config));
    clinear->Close();

    auto cresolution = new TCanvas("resolution", "resolution", 1400, 800);
    cresolution->SetTitle(config_name.c_str());
    std::vector<Double_t> _beam_energies_double;
    for (auto i=0; i < beam_energies.size(); i++){
        _beam_energies_double.push_back(Double_t(beam_energies[i]));
    }
    auto resolution_graph = new TGraph(Mixed_fit_resolution.size(), &_beam_energies_double[0], &Mixed_fit_resolution[0]);
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
    cresolution->SaveAs(Form("../pics/mixed_resolution_%d.png", config));
    cresolution->Close();

    // save HG mu to csv file
    std::ofstream Mixed_mu_csv;
    auto header = "Mean,Sigma,Resolution,FitRange,FitOffset,FitADC";
    Mixed_mu_csv.open(Form("../tmp/Mixed_mu_%d.csv", config));
    Mixed_mu_csv << header << std::endl;
    for (auto i=0; i < Mixed_fit_mu.size(); i++){
        Mixed_mu_csv << Mixed_fit_mu[i] << "," << Mixed_fit_sigma[i] << "," << Mixed_fit_resolution[i] << "," << Mixed_fit_para_range[i] << "," << Mixed_fit_para_offset[i] << "," << Mixed_fit_para_adc[i] << std::endl;
    }
    Mixed_mu_csv.close();


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
