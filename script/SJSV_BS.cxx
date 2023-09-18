#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "TFile.h"
#include "TTree.h"
#include "TLatex.h"
#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TPaveText.h"
#include "csv.h"
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations


int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    std::vector<Double_t> bias_voltages = {
        56.5,
        55.4,
        54.5,
    };

    std::vector<std::string> csv_file_names = {
        "../tmp/HG_mu_5.csv",
        "../tmp/HG_mu_1.csv",
        "../tmp/HG_mu_3.csv",
    };

    std::vector<TGraph*> graphs;
    std::vector<TGraph*> graphs_window;
    TLegend* legend = new TLegend(0.1, 0.7, 0.33, 0.9);
    legend->SetTextSize(0.035);
    TLegend* legend_window = new TLegend(0.1, 0.7, 0.33, 0.9);
    legend_window->SetTextSize(0.035);

    std::vector<Double_t> fit_par_0_list;
    std::vector<Double_t> fit_par_1_list;
    std::vector<Double_t> fit_par_0_err_list;
    std::vector<Double_t> fit_par_1_err_list;

    int window_size = 3;

    for (int i = 0; i < bias_voltages.size(); i++) {
        io::CSVReader<2> _in(csv_file_names[i]);
        _in.read_header(io::ignore_extra_column, "Beam", "ADC");
        Double_t beam, adc;
        std::vector<Double_t> beam_v, adc_v;
        while(_in.read_row(beam, adc)){
            beam_v.push_back(beam);
            adc_v.push_back(adc);
        }

        std::vector<Double_t> _p0_list;
        std::vector<Double_t> _p1_list;
        std::vector<Double_t> _energy_start_list;
        std::vector<Double_t> _energy_end_list;
        std::vector<Double_t> _energy_middle_list;
        std::vector<Double_t> _energy_span_list;
        for (auto i=0; i<=beam_v.size()-window_size; i++){
            auto gtmp =  TGraph(window_size, &beam_v[i], &adc_v[i]);
            auto ftmp = TF1("ftmp", "[0]*x+[1]", beam_v[i], beam_v[i+window_size-1]);
            gtmp.Fit("ftmp", "R", "", beam_v[i], beam_v[i+window_size-1]);
            auto _p0 = ftmp.GetParameter(0);
            auto _p1 = ftmp.GetParameter(1);
            _p0_list.push_back(_p0);
            _p1_list.push_back(_p1);
            _energy_end_list.push_back(beam_v[i+window_size-1]);
            _energy_start_list.push_back(beam_v[i]);
            _energy_middle_list.push_back((beam_v[i+window_size-1]+beam_v[i])/2);
            _energy_span_list.push_back(abs(beam_v[i+window_size-1]-beam_v[i])/2);
            LOG(INFO) << _p0 <<" "<<_p1;
            LOG(INFO) << beam_v[i] << " " << beam_v[i+window_size-1];
        }

        LOG(INFO) << _p0_list.size();

        // use span as x error
        TGraph* graph_window = new TGraphErrors(_p0_list.size(), &_energy_middle_list[0], &_p0_list[0], &_energy_span_list[0], nullptr);
        graph_window->SetMarkerSize(0);
        graph_window->GetXaxis()->SetRangeUser(0,400);
        graph_window->GetYaxis()->SetRangeUser(0,130);
        graph_window->GetXaxis()->SetTitle("Beam Energy [GeV]");
        graph_window->GetYaxis()->SetTitle("Fitted Slope [ADC/GeV]");
        graph_window->SetTitle("");
        // graph_window->GetYaxis()->SetTitleOffset(1.6);
        graph_window->SetMarkerStyle(i+20);
        graph_window->SetLineWidth(3);

        TGraph* graph = new TGraph(beam_v.size(), &beam_v[0], &adc_v[0]);
        TF1 *f1 = new TF1("f1", "[0]*x+[1]", 0, 400);
        f1->SetLineWidth(3);
        // f1->SetLineStyle(i+2);
        graph->SetMarkerStyle(i+20);
        graph->SetMarkerSize(1.5);
        if (i == 2) {
            graph->SetMarkerColor(kBlue);
            f1->SetLineColor(kBlue);
            graph->SetMarkerSize(2);
            graph_window->SetMarkerColor(kBlue);
            //graph_window->SetMarkerSize(2);
            graph_window->SetLineColor(kBlue);
        }
        else {
            graph->SetMarkerColor(i+1);
            f1->SetLineColor(i+1);
            graph_window->SetMarkerColor(i+1);
            graph_window->SetLineColor(i+1);
        }
        graph->SetLineWidth(2);
        graph->SetTitle("");

        graph->GetXaxis()->SetTitle("Beam Energy [GeV]");
        graph->GetYaxis()->SetTitle("ADC");

        

        // leave room for y axis title
        graph->GetYaxis()->SetTitleOffset(1.6);

        graph->Fit("f1", "R", "", 0, 400);
        fit_par_0_list.push_back(f1->GetParameter(0));
        fit_par_1_list.push_back(f1->GetParameter(1));
        fit_par_0_err_list.push_back(f1->GetParError(0));
        fit_par_1_err_list.push_back(f1->GetParError(1));

        graph->GetXaxis()->SetRangeUser(0, 400);
        graph->GetYaxis()->SetRangeUser(0, 40000);
        // force set range
        graph->GetXaxis()->SetLimits(0, 400);
        graph->GetYaxis()->SetLimits(0, 40000);

        // plot line with 59.51x - 172.657
        auto chi2 = f1->GetChisquare();
        auto ndf = f1->GetNDF();
        LOG(INFO) << "Chi2/NDF = " << chi2 << "/" << ndf << " = " << chi2/ndf;

        graphs.push_back(graph);
        graphs_window.push_back(graph_window);

        legend->AddEntry(graph, Form("V_{bias} = %.1f V", bias_voltages[i]), "p");
        legend_window->AddEntry(graph_window, Form("V_{bias} = %.1f V", bias_voltages[i]), "l");

    }

    auto c = new TCanvas("c", "c", 800, 600);
    c->SetGrid();
    for (int i = 0; i < graphs.size(); i++) {
        if (i == 0) {
            graphs[i]->Draw("AP");
        } else {
            graphs[i]->Draw("P same");
        }
    }
    legend->Draw();

    TLatex* latexInfo = new TLatex();
    latexInfo->SetTextSize(0.04);
    latexInfo->SetTextAlign(13);  //align at top
    latexInfo->SetTextFont(62);
    latexInfo->SetNDC();
    latexInfo->DrawLatex(0.35, 0.88, "ADC = a #times Beam Energy + b");
    latexInfo->SetTextSize(0.03);
    latexInfo->DrawLatex(0.35, 0.83, "ADC Linearity of HG Bias Scan");
    latexInfo->DrawLatex(0.35, 0.79, "Hadron Beam, SPS H4");
    latexInfo->DrawLatex(0.35, 0.75, "FoCal-H Prototype 2");

    TLatex* latex1 = new TLatex();
    latex1->SetTextSize(0.035);
    latex1->SetTextAlign(13);  //align at top
    latex1->SetTextFont(42);
    latex1->SetNDC();
    latex1->DrawLatex(0.65, 0.39, Form("a = %.2f #pm %.2f", fit_par_0_list[0], fit_par_0_err_list[0]));
    latex1->DrawLatex(0.65, 0.34, Form("b = %.2f #pm %.2f", fit_par_1_list[0], fit_par_1_err_list[0]));

    TLatex* latex2 = new TLatex();
    latex2->SetTextSize(0.035);
    latex2->SetTextAlign(13);  //align at top
    latex2->SetTextFont(42);
    latex2->SetTextColor(kRed);
    latex2->SetNDC();
    latex2->DrawLatex(0.65, 0.29, Form("a = %.2f #pm %.2f", fit_par_0_list[1], fit_par_0_err_list[1]));
    latex2->DrawLatex(0.65, 0.24, Form("b = %.2f #pm %.2f", fit_par_1_list[1], fit_par_1_err_list[1]));

    TLatex* latex3 = new TLatex();
    latex3->SetTextSize(0.035);
    latex3->SetTextAlign(13);  //align at top
    latex3->SetTextFont(42);
    latex3->SetTextColor(kBlue);
    latex3->SetNDC();
    latex3->DrawLatex(0.65, 0.19, Form("a = %.2f #pm %.2f", fit_par_0_list[2], fit_par_0_err_list[2]));
    latex3->DrawLatex(0.65, 0.14, Form("b = %.2f #pm %.2f", fit_par_1_list[2], fit_par_1_err_list[2]));

    TLatex* water_mark = new TLatex();
    water_mark->SetTextSize(0.15);
    water_mark->SetTextAlign(13);  //align at top
    // water_mark->SetTextFont(42);
    water_mark->SetTextColorAlpha(kGray+2, 0.1);
    water_mark->SetNDC();
    water_mark->SetTextAngle(30);
    water_mark->DrawLatex(0.15, 0.25, "Very Preliminary");
    
    c->SaveAs("../pics/HG_lin.png");
    c->Close();

    auto c2 = new TCanvas("c2", "c2", 800, 450);
    c2->SetGrid();
    LOG(INFO) << graphs_window.size();
    for (int i = 0; i < graphs_window.size(); i++) {
        if (i == 0) {
            graphs_window[i]->Draw("AP");
        } else {
            graphs_window[i]->Draw("P same");
        }
    }
    legend_window->SetY1(0.1);
    legend_window->SetY2(0.35);
    legend_window->Draw();
    water_mark->DrawLatex(0.25, 0.25, "Very Preliminary");
    auto text_vspace = 0.06;
    auto start_y = 0.28;
    latexInfo->SetTextSize(0.04);
    latexInfo->DrawLatex(0.34, start_y, "ADC Response Slope with Window Size = 3");
    start_y -= text_vspace;
    latexInfo->DrawLatex(0.34, start_y, "Hadron Beam, SPS H4");
    start_y -= text_vspace;
    latexInfo->DrawLatex(0.34, start_y, "FoCal-H Prototype 2");
    start_y -= text_vspace;

    c2->SaveAs("../pics/HG_lin_window.png");
    c2->Close();
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