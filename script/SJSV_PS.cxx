#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TLegend.h"
#include "TLatex.h"

#include "csv.h"
#include "easylogging++.h"


void set_easylogger(); // set easylogging++ configurations

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    // * --- Loading and setting data ---
    // * ------------------------------------------------------------
    int config = 2;

    std::vector<std::string> csv_file_names;
    std::vector<Int_t> run_module_number;
    std::string config_info;
    std::string output_png_name;
    
    const std::vector<Int_t> center_module_number = {5};
    const std::vector<Int_t> corner_module_number = {1,3,7,9};
    const std::vector<Int_t> edge_module_number = {2,4,6,8};

    std::string common_info_line1 = "FoCal-H Prototype 2";
    std::string common_info_line2 = "SPS H4 Beam Test";
    std::string common_info_line3 = "September 2023";

    Double_t    hist_max_x;
    Double_t    hist_min_x;
    Int_t       hist_bin_num;
    Double_t    hist_max_y;

    switch (config)
    {
    case 1: {
        config_info = "Position Scan for 350 GeV Hadrons";
        csv_file_names = std::vector<std::string> {
            "../tmp/HL_Correlation/Mixed_ADC_sum_run035.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run030.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run031.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run036.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run037.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run032.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run029.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run034.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run033.csv",
        };
        run_module_number = std::vector<Int_t> {
            1,2,3,4,5,6,7,8,9
        };
        output_png_name = "PS_Hadron_Mixed_adc_hist.png";
        hist_max_x = 80000;
        hist_min_x = 0;
        hist_bin_num = 1000;
        hist_max_y = 0.01;
        break;
    }
    case 2: {
        config_info = "Position Scan for 100 GeV Electrons";
        csv_file_names = std::vector<std::string> {
            "../tmp/HL_Correlation/Mixed_ADC_sum_run040.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run045.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run044.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run039.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run038.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run043.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run046.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run041.csv",
            "../tmp/HL_Correlation/Mixed_ADC_sum_run042.csv",
        };
        run_module_number = std::vector<Int_t> {
            1,2,3,4,5,6,7,8,9
        };
        output_png_name = "PS_Electron_Mixed_adc_hist.png";
        hist_max_x = 18000;
        hist_min_x = 0;
        hist_bin_num = 1000;
        hist_max_y = 0.012;
        break;
    }
    default:
        break;
    }
    LOG(INFO) << "Config: " << config_info;
    // * ------------------------------------------------------------


    std::vector<std::vector<Double_t>*> Mixed_adc_list;
    std::vector<TH1D*> Mixed_adc_hist_list;

    std::vector<Int_t> color_list;
    // give color according to module location
    Int_t _center_cnt = 0;
    Int_t _corner_cnt = 0;
    Int_t _edge_cnt = 0;
    for (auto module_number : run_module_number){
        if (std::find(center_module_number.begin(), center_module_number.end(), module_number) != center_module_number.end()){
            color_list.push_back(kRed);
            _center_cnt++;
        }
        else if (std::find(corner_module_number.begin(), corner_module_number.end(), module_number) != corner_module_number.end()){
            color_list.push_back(kOrange + 1 + _corner_cnt*2);
            _corner_cnt++;
        }
        else if (std::find(edge_module_number.begin(), edge_module_number.end(), module_number) != edge_module_number.end()){
            color_list.push_back(kGreen + 1 + _edge_cnt);
            _edge_cnt++;
        }
        else{
            color_list.push_back(kBlack);
        }
    }
    
    for (auto csv_file_name : csv_file_names){
        std::vector<Double_t>* Mixed_adc = new std::vector<Double_t>;
        io::CSVReader<1> in(csv_file_name);
        in.read_header(io::ignore_extra_column, "Mixed_ADC_sum");
        Double_t Mixed_ADC_sum;
        while(in.read_row(Mixed_ADC_sum)){
            Mixed_adc->push_back(Mixed_ADC_sum);
        }
        Mixed_adc_list.push_back(Mixed_adc);

        auto hist_name = csv_file_name.substr(csv_file_name.find("run")+3,3);
        auto hist_title = csv_file_name.substr(csv_file_name.find("run")+3,3);
        auto hist = new TH1D(hist_name.c_str(), hist_title.c_str(), hist_bin_num, hist_min_x, hist_max_x);
        for (auto Mixed_adc_value : *Mixed_adc){
            hist->Fill(Mixed_adc_value);
        }
        hist->Scale(1.0/hist->Integral());
        hist->GetYaxis()->SetRangeUser(0, hist_max_y);
        hist->SetStats(0);
        hist->SetTitle("");
        hist->SetLineWidth(3);
        hist->GetXaxis()->SetTitle("ADC Sum");
        hist->GetYaxis()->SetTitle("Normalized Counts"); 
        Mixed_adc_hist_list.push_back(hist);
    }

    auto canvas = new TCanvas("canvas", "canvas", 1200, 800);
    auto legend = new TLegend(0.75, 0.55, 0.9, 0.9);
    for (auto i = 0; i < Mixed_adc_list.size(); i++){
        Mixed_adc_hist_list[i]->SetLineColor(color_list[i]);
        Mixed_adc_hist_list[i]->Draw(i==0?"":"same");
        legend->AddEntry(Mixed_adc_hist_list[i], ("Module "+std::to_string(run_module_number[i])+" (" + csv_file_names[i].substr(csv_file_names[i].find("run")+3,3) + ")").c_str(), "l");
    }

    canvas->SetGrid();
    legend->Draw();

    TLatex* text_line0 = new TLatex(0.75, 0.85, config_info.c_str());
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

    text_line0->Draw();
    text_line1->Draw();
    text_line2->Draw();
    text_line3->Draw();

    TLatex* water_mark = new TLatex(0.2, 0.2, "Very Preliminary");
    water_mark->SetNDC();
    water_mark->SetTextColorAlpha(kGray+2, 0.1);
    water_mark->SetTextAngle(30);
    water_mark->SetTextSize(0.15);
    water_mark->Draw();

    canvas->SaveAs(("../pics/" + output_png_name).c_str());

    canvas->Close();
    delete canvas;


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