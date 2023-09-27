#include "TH1.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TStyle.h"
#include "TF1.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "csv.h"
#include "easylogging++.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

void set_easylogger(); // set easylogging++ configurations

void set_alice_style(TStyle *style);

Double_t energy_resolution_func (Double_t *x, Double_t *par){
    Double_t p0 = par[0];
    Double_t p1 = par[1];
    Double_t p2 = par[2];
    return sqrt(pow((p0/sqrt(x[0])),2) + pow(p1/(x[0]),2) + pow(p2,2));
}

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    std::string filename_reference = "../data/Data_May_Reso_HG_Mixed_Reso.txt";
    std::ifstream file_rado_May_Mixed(filename_reference);

    std::vector<double> rado_May_Mixed_energy, rado_May_Mixed_reso, rado_May_Mixed_reso_err;
    double _rado_May_Mixed_energy, _rado_May_Mixed_reso, _rado_May_Mixed_reso_err;
    auto delimiter = ',';
    auto line_ending = '\n';

    std::string line;
    std::getline(file_rado_May_Mixed, line, line_ending);
    std::stringstream ss(line);
    std::string token;
    ss = std::stringstream(line);
    while (std::getline(ss, token, delimiter)) {
        _rado_May_Mixed_energy = std::stod(token);
        rado_May_Mixed_energy.push_back(_rado_May_Mixed_energy);
    }

    std::getline(file_rado_May_Mixed, line, line_ending);
    ss = std::stringstream(line);
    while (std::getline(ss, token, delimiter)) {
        _rado_May_Mixed_reso = std::stod(token);
        rado_May_Mixed_reso.push_back(_rado_May_Mixed_reso);
    }

    std::getline(file_rado_May_Mixed, line, line_ending);
    std::getline(file_rado_May_Mixed, line, line_ending);
    ss = std::stringstream(line);
    while (std::getline(ss, token, delimiter)) {
        _rado_May_Mixed_reso_err = std::stod(token);
        rado_May_Mixed_reso_err.push_back(_rado_May_Mixed_reso_err);
    }


    std::vector<std::string> filename_list = {
        "../tmp/Mixed_mu_1.csv",
        "../tmp/Mixed_mu_3.csv",
        "../tmp/Mixed_mu_2.csv",
    };

    std::vector<std::string> file_legend = {
        "V_{bias} = 56.5 V",
        "V_{bias} = 55.5 V",
        "V_{bias} = 54.5 V",
    };

    std::vector<Int_t> file_color_list = {
        kRed,
        kBlue,
        kTeal + 2,
    };

    std::vector<TGraphErrors*> g_list;
    std::vector<TGraphErrors*> g_list_mean;

    Double_t fit_info_box_x_min = 0.7;
    Double_t fit_info_box_x_max = 0.9;
    Double_t fit_info_box_y_max = 0.9;
    Double_t fit_info_box_y_step = 0.15;

    for (auto _filename: filename_list){
        io::CSVReader<6> in(_filename);
        in.read_header(io::ignore_extra_column, "Mean", "Sigma", "Resolution", "FitRange", "FitOffset", "FitADC");

        std::vector<Double_t> mean_vec;
        std::vector<Double_t> sigma_vec;
        std::vector<Double_t> resolution_vec;
        std::vector<Double_t> fit_range_vec;
        std::vector<Double_t> fit_offset_vec;
        std::vector<Double_t> fit_adc_vec;

        Double_t mean, sigma, resolution, fit_range, fit_offset, fit_adc;
        while (in.read_row(mean, sigma, resolution, fit_range, fit_offset, fit_adc)) {
            mean_vec.push_back(mean);
            sigma_vec.push_back(sigma);
            resolution_vec.push_back(resolution);
            fit_range_vec.push_back(fit_range);
            fit_offset_vec.push_back(fit_offset);
            fit_adc_vec.push_back(fit_adc);
        }
        // print vectors' size
        LOG(INFO) << "mean_vec size: " << mean_vec.size();
        LOG(INFO) << "sigma_vec size: " << sigma_vec.size();
        LOG(INFO) << "resolution_vec size: " << resolution_vec.size();
        LOG(INFO) << "resolution_vec #0: " << resolution_vec[0];
        LOG(INFO) << "fit_range_vec size: " << fit_range_vec.size();
        LOG(INFO) << "fit_offset_vec size: " << fit_offset_vec.size();
        LOG(INFO) << "fit_adc_vec size: " << fit_adc_vec.size();

        // close csv file
        //in.close();

        // find all resolutions for each fit_adc
        std::vector<Double_t> fit_adc_list;
        std::vector<std::vector<Double_t>> resolution_list;
        std::vector<std::vector<Double_t>> mean_list;
        std::vector<std::vector<Double_t>> sigma_list;
        LOG(INFO) << "Sorting resolutions ...";
        for (int i = 0; i < fit_adc_vec.size(); i++) {
            LOG(DEBUG) << "i: " << i;
            if (std::find(fit_adc_list.begin(), fit_adc_list.end(), fit_adc_vec[i]) == fit_adc_list.end()) {
                fit_adc_list.push_back(fit_adc_vec[i]);
                std::vector<Double_t> _resolution_vec;
                std::vector<Double_t> _mean_vec;
                std::vector<Double_t> _sigma_vec;
                for (int j = 0; j < fit_adc_vec.size(); j++) {
                    LOG(DEBUG) << "j: " << j;
                    if (fit_adc_vec[j] == fit_adc_vec[i]) {
                        LOG(DEBUG) << "fit_adc_vec[j]: " << fit_adc_vec[j];
                        LOG(DEBUG) << "fit_adc_vec[i]: " << fit_adc_vec[i];
                        LOG(DEBUG) << "resolution_vec[j]: " << resolution_vec[j];
                        LOG(DEBUG) << "mean_vec[j]: " << mean_vec[j];
                        LOG(DEBUG) << "sigma_vec[j]: " << sigma_vec[j];
                        _resolution_vec.push_back(resolution_vec[j]);
                        _mean_vec.push_back(mean_vec[j]);
                        _sigma_vec.push_back(sigma_vec[j]);
                    }
                }
                resolution_list.push_back(_resolution_vec);
                mean_list.push_back(_mean_vec);
                sigma_list.push_back(_sigma_vec);
            }
        }

        std::vector<Double_t> resolution_mean_list;
        std::vector<Double_t> resolution_error_list;
        LOG(INFO) << "Calculating mean and error ...";
        for (int i = 0; i < resolution_list.size(); i++) {
            Double_t resolution_mean = 0;
            Double_t resolution_error = 0;
            for (int j = 0; j < resolution_list[i].size(); j++) {
                resolution_mean += resolution_list[i][j];
            }
            resolution_mean /= resolution_list[i].size();
            resolution_mean_list.push_back(resolution_mean);
            for (int j = 0; j < resolution_list[i].size(); j++) {
                resolution_error += pow(resolution_list[i][j] - resolution_mean, 2);
            }
            resolution_error /= resolution_list[i].size();
            resolution_error = sqrt(resolution_error);
            resolution_error_list.push_back(resolution_error);
        }

        std::vector<Double_t> mean_mean_list;
        std::vector<Double_t> mean_error_list;
        LOG(INFO) << "Calculating mean and error ...";
        for (int i = 0; i < mean_list.size(); i++) {
            Double_t mean_mean = 0;
            Double_t mean_error = 0;
            for (int j = 0; j < mean_list[i].size(); j++) {
                mean_mean += mean_list[i][j];
            }
            mean_mean /= mean_list[i].size();
            mean_mean_list.push_back(mean_mean);
            for (int j = 0; j < mean_list[i].size(); j++) {
                mean_error += pow(mean_list[i][j] - mean_mean, 2);
            }
            mean_error /= mean_list[i].size();
            mean_error = sqrt(mean_error);
            mean_error_list.push_back(mean_error);
        }

        auto _g = new TGraphErrors(fit_adc_list.size(), &fit_adc_list[0], &resolution_mean_list[0], 0, &resolution_error_list[0]);
        g_list.push_back(_g);

        auto _g_mean = new TGraphErrors(fit_adc_list.size(), &fit_adc_list[0], &mean_mean_list[0], 0, &mean_error_list[0]);
        g_list_mean.push_back(_g_mean);
    }

    

    LOG(INFO) << "Plotting ...";
    auto c1 = new TCanvas("c1", "c1", 1400, 800);
    auto legend = new TLegend(0.55, 0.7, 0.7, 0.9);


    std::vector<Double_t> fit_p0_list;
    std::vector<Double_t> fit_p1_list;
    std::vector<Double_t> fit_p2_list;
    std::vector<Double_t> fit_NDF_list;
    std::vector<Double_t> fit_chi2_list;

    // draw reference
    auto _g_ref = new TGraphErrors(rado_May_Mixed_energy.size(), &rado_May_Mixed_energy[0], &rado_May_Mixed_reso[0], 0, &rado_May_Mixed_reso_err[0]);
    _g_ref->SetTitle("Energy Resolution");
    _g_ref->GetXaxis()->SetTitle("Beam Energy [GeV]");
    _g_ref->GetYaxis()->SetTitle("Energy Resolution");
    _g_ref->SetMarkerStyle(20);
    _g_ref->SetMarkerSize(1.6);
    _g_ref->SetMarkerColor(kBlack);
    _g_ref->SetLineColor(kBlack);
    _g_ref->SetLineWidth(3);
    _g_ref->GetXaxis()->SetRangeUser(0, 400);
    _g_ref->GetYaxis()->SetRangeUser(0.1, 0.4);
    _g_ref->GetXaxis()->SetLimits(0, 400);
    _g_ref->GetYaxis()->SetLimits(0.1, 0.4);
    legend->AddEntry(_g_ref, "May CAEN", "ple");
    _g_ref->Draw("AP");

    // fit
    TF1 *fit_func_ref = new TF1("fit_func_ref", energy_resolution_func, 30, 370, 3);
    fit_func_ref->SetParNames("p0", "p1", "p2");
    fit_func_ref->SetParameters(1, 0, 0.15);
    fit_func_ref->SetLineColor(kBlack);
    fit_func_ref->SetLineWidth(3);
    fit_func_ref->SetLineStyle(2);
    _g_ref->Fit("fit_func_ref", "R");

    fit_p0_list.push_back(fit_func_ref->GetParameter(0));
    fit_p1_list.push_back(fit_func_ref->GetParameter(1));
    fit_p2_list.push_back(fit_func_ref->GetParameter(2));
    fit_NDF_list.push_back(fit_func_ref->GetNDF());
    fit_chi2_list.push_back(fit_func_ref->GetChisquare());

    auto fit_box_y_high_ref = fit_info_box_y_max;
    auto info_box_fit_func_ref = new TPaveText(fit_info_box_x_min, fit_info_box_y_max - fit_info_box_y_step, fit_info_box_x_max, fit_box_y_high_ref, "NDC");
    info_box_fit_func_ref->AddText(Form("p_{0} = %.3f #pm %.3f", fit_func_ref->GetParameter(0), fit_func_ref->GetParError(0)));
    info_box_fit_func_ref->AddText(Form("p_{1} = %.3f #pm %.3f", fit_func_ref->GetParameter(1), fit_func_ref->GetParError(1)));
    info_box_fit_func_ref->AddText(Form("p_{2} = %.3f #pm %.3f", fit_func_ref->GetParameter(2), fit_func_ref->GetParError(2)));
    info_box_fit_func_ref->AddText(Form("#chi^{2} / NDF = %.3f / %d", fit_func_ref->GetChisquare(), fit_func_ref->GetNDF()));
    info_box_fit_func_ref->SetTextFont(42);
    info_box_fit_func_ref->SetTextSize(0.03);
    info_box_fit_func_ref->SetTextColor(kBlack);
    info_box_fit_func_ref->SetFillColorAlpha(kWhite, 1);
    info_box_fit_func_ref->SetBorderSize(1);
    info_box_fit_func_ref->Draw();

    for (auto i = 0; i < g_list.size(); i++){
        auto _graph = g_list[i];
        _graph->SetTitle("Energy Resolution");
        _graph->GetXaxis()->SetTitle("Beam Energy [GeV]");
        _graph->GetYaxis()->SetTitle("Energy Resolution");
        _graph->SetMarkerStyle(i + 21);
        _graph->SetMarkerSize(1.8);
        _graph->SetMarkerColor(file_color_list[i]);
        _graph->SetLineColor(file_color_list[i]);
        _graph->SetLineWidth(3);
        _graph->GetXaxis()->SetRangeUser(0, 400);
        _graph->GetYaxis()->SetRangeUser(0.1, 0.4);
        _graph->GetXaxis()->SetLimits(0, 400);
        _graph->GetYaxis()->SetLimits(0.1, 0.4);

        TF1 *fit_func = new TF1("fit_func", energy_resolution_func, 30, 370, 3);
        fit_func->SetParNames("p0", "p1", "p2");
        fit_func->SetParameters(1, 0, 0.15);
        fit_func->SetLineColor(file_color_list[i]);
        fit_func->SetLineWidth(3);
        fit_func->SetLineStyle(2);

        legend->AddEntry(_graph, file_legend[i].c_str(), "ple");
        _graph->Draw("P SAME");

        _graph->Fit("fit_func", "R");
        fit_p0_list.push_back(fit_func->GetParameter(0));
        fit_p1_list.push_back(fit_func->GetParameter(1));
        fit_p2_list.push_back(fit_func->GetParameter(2));
        fit_NDF_list.push_back(fit_func->GetNDF());
        fit_chi2_list.push_back(fit_func->GetChisquare());

        auto fit_box_y_high = fit_info_box_y_max - fit_info_box_y_step * (i+1);
        auto info_box_fit_func = new TPaveText(fit_info_box_x_min, fit_info_box_y_max - fit_info_box_y_step * (i+2), fit_info_box_x_max, fit_box_y_high, "NDC");
        info_box_fit_func->AddText(Form("p_{0} = %.3f #pm %.3f", fit_func->GetParameter(0), fit_func->GetParError(0)));
        info_box_fit_func->AddText(Form("p_{1} = %.3f #pm %.3f", fit_func->GetParameter(1), fit_func->GetParError(1)));
        info_box_fit_func->AddText(Form("p_{2} = %.3f #pm %.3f", fit_func->GetParameter(2), fit_func->GetParError(2)));
        info_box_fit_func->AddText(Form("#chi^{2} / NDF = %.3f / %d", fit_func->GetChisquare(), fit_func->GetNDF()));
        info_box_fit_func->SetTextFont(42);
        info_box_fit_func->SetTextSize(0.03);
        info_box_fit_func->SetTextColor(file_color_list[i]);
        info_box_fit_func->SetFillColorAlpha(kWhite, 1);
        info_box_fit_func->SetBorderSize(1);
        info_box_fit_func->Draw();
    }

    legend->Draw();

    set_alice_style(gStyle);

    // add grid with 2 width and black color
    c1->SetGrid();

    auto info_box_fit_func = new TPaveText(fit_info_box_x_min - 0.03, fit_info_box_y_max - fit_info_box_y_step * (g_list.size()+1), fit_info_box_x_max, fit_info_box_y_max - fit_info_box_y_step * g_list.size(), "NDC");
    info_box_fit_func->AddText("fitting with #sqrt{(#frac{p_{0}}{#sqrt{E}})^{2} + (#frac{p_{1}}{E})^{2} + p_{2}^{2}}");
    info_box_fit_func->SetTextFont(42);
    info_box_fit_func->SetTextSize(0.03);
    info_box_fit_func->SetTextColor(kBlack);
    info_box_fit_func->SetFillColorAlpha(kWhite, 0);
    info_box_fit_func->SetBorderSize(0);
    // info_box_fit_func->Draw();

    // add figure info
    TLatex *latexInfo = new TLatex();
    latexInfo->SetTextSize(0.03);
    latexInfo->SetTextAlign(33);  //align at top
    latexInfo->SetNDC();
    latexInfo->SetTextFont(62);
    latexInfo->SetTextColor(kBlack);
    latexInfo->DrawLatex(0.55, 0.88, "Crystal Ball Fitting with 1 - 3 sigma");
    latexInfo->DrawLatex(0.55, 0.84, "-1500 - +1500 ADC offset");
    latexInfo->DrawLatex(0.55, 0.80, "FoCal-H Prototype 2");
    latexInfo->DrawLatex(0.55, 0.76, "SPS H4, September 2023");

    // add watermark
    TLatex *watermark = new TLatex();
    watermark->SetTextSize(0.16);
    watermark->SetNDC();
    watermark->SetTextFont(42);
    watermark->SetTextColorAlpha(kGray, 0.2);
    watermark->SetTextAngle(35);
    watermark->DrawLatex(0.25, 0.15, "Very Preliminary");


    c1->SaveAs("../pics/energy_resolution.png");
    LOG(INFO) << "Done";    

    c1->Close();
    delete c1;

    auto c2 = new TCanvas("c2", "c2", 1200, 1200);
    auto legend_mean = new TLegend(0.15, 0.7, 0.3, 0.9);

    for (int i = 0; i < g_list_mean.size(); i++) {
        auto _graph = g_list_mean[i];
        _graph->SetTitle("Mean");
        _graph->GetXaxis()->SetTitle("Beam Energy [GeV]");
        _graph->GetYaxis()->SetTitle("Mean");
        _graph->SetMarkerStyle(i + 21);
        _graph->SetMarkerSize(1.8);
        _graph->SetMarkerColor(file_color_list[i]);
        _graph->SetLineColor(file_color_list[i]);
        _graph->SetLineWidth(3);
        _graph->GetXaxis()->SetRangeUser(0, 400);
        _graph->GetYaxis()->SetRangeUser(0, 45000);
        _graph->GetXaxis()->SetLimits(0, 400);
        _graph->GetYaxis()->SetLimits(0, 45000);

        legend_mean->AddEntry(_graph, file_legend[i].c_str(), "ple");
        if (i == 0) {
            _graph->Draw("AP");
        } else {
            _graph->Draw("P SAME");
        }
    }

    legend_mean->Draw();

    set_alice_style(gStyle);

    c2->SetGrid();

    c2->SaveAs("../pics/energy_mean.png");
    LOG(INFO) << "Done";

    c2->Close();
    delete c2;

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

void set_alice_style(TStyle *style){
    // CERN ALICE style
    style->SetOptStat(0);
    style->SetOptFit(0);
    style->SetOptTitle(0);
    style->SetPadTickX(1);
    style->SetPadTickY(1);
    style->SetPadLeftMargin(0.15);
    style->SetPadRightMargin(0.05);
    style->SetPadTopMargin(0.05);
    style->SetPadBottomMargin(0.15);
    style->SetTitleOffset(1.2, "x");
    style->SetTitleOffset(1.2, "y");
    style->SetTitleOffset(1.2, "z");
    style->SetTitleSize(0.05, "x");
    style->SetTitleSize(0.05, "y");
    style->SetTitleSize(0.05, "z");
    style->SetTitleFont(42, "x");
    style->SetTitleFont(42, "y");
    style->SetTitleFont(42, "z");
    style->SetLabelSize(0.05, "x");
    style->SetLabelSize(0.05, "y");
    style->SetLabelSize(0.05, "z");
    style->SetLabelFont(42, "x");
    style->SetLabelFont(42, "y");
    style->SetLabelFont(42, "z");
    style->SetLegendBorderSize(0);
    style->SetLegendFillColor(0);
    style->SetLegendFont(42);
    style->SetLegendTextSize(0.03);
    style->SetMarkerStyle(20);
    style->SetMarkerSize(1.2);
    style->SetMarkerColor(kBlack);
    style->SetLineWidth(2);
    style->SetLineColor(kBlack);
    style->SetHistLineWidth(2);
}