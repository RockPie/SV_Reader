#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "TFile.h"
#include "TTree.h"
#include "TLatex.h"
#include "TF1.h"
#include "TGraph.h"
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
    TLegend* legend = new TLegend(0.1, 0.7, 0.33, 0.9);
    legend->SetTextSize(0.035);

    std::vector<Double_t> fit_par_0_list;
    std::vector<Double_t> fit_par_1_list;
    std::vector<Double_t> fit_par_0_err_list;
    std::vector<Double_t> fit_par_1_err_list;


    for (int i = 0; i < bias_voltages.size(); i++) {
        io::CSVReader<2> _in(csv_file_names[i]);
        _in.read_header(io::ignore_extra_column, "Beam", "ADC");
        Double_t beam, adc;
        std::vector<Double_t> beam_v, adc_v;
        while(_in.read_row(beam, adc)){
            beam_v.push_back(beam);
            adc_v.push_back(adc);
        }

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
        }
        else {
            graph->SetMarkerColor(i+1);
            f1->SetLineColor(i+1);
        }
        graph->SetLineWidth(2);
        graph->SetTitle("");

        graph->GetXaxis()->SetTitle("Beam Energy [GeV]");
        graph->GetYaxis()->SetTitle("ADC");

        graph->GetXaxis()->SetRangeUser(0, 400);
        graph->GetYaxis()->SetRangeUser(0, 40000);
        // force set range
        graph->GetXaxis()->SetLimits(0, 400);
        graph->GetYaxis()->SetLimits(0, 40000);

        // leave room for y axis title
        graph->GetYaxis()->SetTitleOffset(1.6);

        graph->Fit("f1", "R", "", 0, 400);
        fit_par_0_list.push_back(f1->GetParameter(0));
        fit_par_1_list.push_back(f1->GetParameter(1));
        fit_par_0_err_list.push_back(f1->GetParError(0));
        fit_par_1_err_list.push_back(f1->GetParError(1));

        // plot line with 59.51x - 172.657
        

        auto chi2 = f1->GetChisquare();
        auto ndf = f1->GetNDF();
        LOG(INFO) << "Chi2/NDF = " << chi2 << "/" << ndf << " = " << chi2/ndf;

        graphs.push_back(graph);

        legend->AddEntry(graph, Form("V_{bias} = %.1f V", bias_voltages[i]), "p");

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
    
    c->SaveAs("../pics/HG_mu.png");
    c->Close();
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