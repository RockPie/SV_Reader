#include <iostream>
#include <unistd.h>
#include "TCanvas.h" 
#include "TVectorD.h"
#include "easylogging++.h"
#include "SJSV_pcapreader.h"
#include "SJSV_eventbuilder.h"

void set_easylogger(); // set easylogging++ configurations


int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    set_easylogger();

    int run_number = 37;
    bool save_to_rootfile = true;
    bool save_to_png = false;
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

    auto root_file_name = Form("../tmp/parsed_Run0%dv.root", run_number);
    auto export_file_name = Form("../tmp/HL_Corr_Run0%dv.root", run_number);
    std::string filename_mapping_csv = "../data/config/Mapping_tb2023Sep_VMM2.csv";

    LOG(INFO) << "Run number: " << run_number;

    SJSV_eventbuilder eventbuilder;
    eventbuilder.load_parsed_data(root_file_name);
    eventbuilder.load_mapping_file(filename_mapping_csv);
    eventbuilder.set_bcid_cycle(bcid_cycle);
    eventbuilder.set_tdc_slope(tdc_slope);
    eventbuilder.reconstruct_event_list(reconstructed_threshold_time_ns);

    std::vector<int> cell_id_list_global;
    std::vector<Int_t> cell_lg_list_global;
    std::vector<Int_t> cell_hg_list_global;

    auto parsed_hit_number = eventbuilder.get_parsed_hit_number();
    auto parsed_event_number = eventbuilder.get_parsed_event_number();
    LOG(INFO) << "Parsed hit number: " << parsed_hit_number;
    LOG(INFO) << "Parsed event number: " << parsed_event_number;
    auto progress_divider = parsed_event_number / 10;
    for (auto event_index=0; event_index < eventbuilder.get_parsed_event_number(); event_index++){
        if (event_index % progress_divider == 0){
            LOG(INFO) << "Progress: " << event_index / progress_divider * 10 << "%";
        }
        // LOG(DEBUG) << event_index%progress_divider;
        auto event = eventbuilder.event_at(event_index);
        auto hit_num = event.frames_ptr.size();
        std::vector<int> cell_id_list;
        std::vector<Int_t> cell_hg_list;
        std::vector<Int_t> cell_lg_list;
        int mapped_hit_cnt = 0;
        int total_hit_cnt = 0;
        for (auto hit: event.frames_ptr){
            auto coords = eventbuilder.get_frame_coord(*hit);
            total_hit_cnt++;
            if (coords.at(0) == -1 || coords.at(1) == -1){
                // LOG(WARNING) << hit->uni_channel;
                continue;
            }
            mapped_hit_cnt++;
            int map_cell_id = coords.at(0) * 210 + coords.at(1);
            bool founded = false;
            int index = 0;
 
            for (auto cell_index=0; cell_index < cell_id_list.size(); cell_index++){
                if (cell_id_list.at(cell_index) == map_cell_id){
                    founded = true;
                    index = cell_index;
                    break;
                }
            }
            if (!founded){
                cell_id_list.push_back(map_cell_id);
                if (eventbuilder.is_frame_HG(*hit)){
                    cell_hg_list.push_back(hit->adc);
                    cell_lg_list.push_back(-1);
                } else {
                    cell_hg_list.push_back(-1);
                    cell_lg_list.push_back(hit->adc);
                }
            }
            else {
                if (eventbuilder.is_frame_HG(*hit)){
                    cell_hg_list.at(index) = hit->adc;
                } else {
                    cell_lg_list.at(index) = hit->adc;
                }
            }
        }
        for (auto cell_index=0; cell_index < cell_id_list.size(); cell_index++){
            if (cell_hg_list.at(cell_index) == -1 || cell_lg_list.at(cell_index) == -1){
                cell_id_list.erase(cell_id_list.begin() + cell_index);
                cell_hg_list.erase(cell_hg_list.begin() + cell_index);
                cell_lg_list.erase(cell_lg_list.begin() + cell_index);
            }
        }
        cell_id_list_global.insert(cell_id_list_global.end(), cell_id_list.begin(),cell_id_list.end());
        cell_hg_list_global.insert(cell_hg_list_global.end(), cell_hg_list.begin(),cell_hg_list.end());
        cell_lg_list_global.insert(cell_lg_list_global.end(), cell_lg_list.begin(),cell_lg_list.end());
        cell_id_list.clear();
        cell_hg_list.clear();
        cell_lg_list.clear();
    }


    // delete cell with -1 HG or -1 LG


    LOG(INFO) << "Valid cell number: " << cell_id_list_global.size();

    // get unique cell_id
    std::vector<int> cell_id_list_global_unique;
    for (auto cell_id: cell_id_list_global){
        bool founded = false;
        for (auto cell_id_unique: cell_id_list_global_unique){
            if (cell_id_unique == cell_id){
                founded = true;
                break;
            }
        }
        if (!founded){
            cell_id_list_global_unique.push_back(cell_id);
        }
    }

    LOG(INFO) << "Unique cell number: " << cell_id_list_global_unique.size();

    // create list of hg and lg for each cell id
    std::vector<std::vector<Int_t>> cell_hg_list_global_unique;
    std::vector<std::vector<Int_t>> cell_lg_list_global_unique;
    for (auto cell_id_unique: cell_id_list_global_unique){
        std::vector<Int_t> cell_hg_list;
        std::vector<Int_t> cell_lg_list;
        for (auto cell_index=0; cell_index < cell_id_list_global.size(); cell_index++){
            if (cell_id_list_global.at(cell_index) == cell_id_unique){
                cell_hg_list.push_back(cell_hg_list_global.at(cell_index));
                cell_lg_list.push_back(cell_lg_list_global.at(cell_index));
            }
        }
        cell_hg_list_global_unique.push_back(cell_hg_list);
        cell_lg_list_global_unique.push_back(cell_lg_list);
    }

    auto analysis_file = new TFile(export_file_name, "RECREATE");

    // draw correlation
    for (auto cell_i=0; cell_i < cell_id_list_global_unique.size(); cell_i++) {
        auto canvas = new TCanvas(Form("canvas_%d", cell_i), Form("canvas_%d", cell_i), 800, 600);
        auto cell_id = cell_id_list_global_unique.at(cell_i);
        auto cell_hg_list = cell_hg_list_global_unique.at(cell_i);
        auto cell_lg_list = cell_lg_list_global_unique.at(cell_i);
        auto cell_hg_list_size = cell_hg_list.size();
        auto cell_lg_list_size = cell_lg_list.size();
        auto cell_hg_list_array = new Double_t[cell_hg_list_size];
        auto cell_lg_list_array = new Double_t[cell_lg_list_size];
        for (auto i=0; i < cell_hg_list_size; i++){
            cell_hg_list_array[i] = cell_hg_list.at(i);
        }
        for (auto i=0; i < cell_lg_list_size; i++){
            cell_lg_list_array[i] = cell_lg_list.at(i);
        }

        // draw correlation
        auto graph = new TGraph(cell_hg_list_size, cell_hg_list_array, cell_lg_list_array);
        graph->SetTitle(Form("Cell ID: %d", cell_id));
        graph->GetXaxis()->SetTitle("HG");
        graph->GetYaxis()->SetTitle("LG");
        graph->SetMarkerStyle(20);
        graph->SetMarkerSize(0.3);

        graph->GetXaxis()->SetRangeUser(0, 1024);
        graph->GetYaxis()->SetRangeUser(0, 200);
        graph->GetXaxis()->SetLimits(0, 1024);
        graph->GetYaxis()->SetLimits(0, 200);

        graph->Draw("AP");
        canvas->Write();
        if (save_to_png){
            canvas->SaveAs(Form("../tmp/HL_Correlation/HL_Corr_Run0%dv_CellID%d.png", run_number, cell_id));
        }
        canvas->Close();
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