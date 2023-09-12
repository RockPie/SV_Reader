#pragma once

#include "easylogging++.h"

#include "TFile.h"
#include "TTree.h"

#include "TGraph.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TAxis.h"  
#include "TLegend.h"
#include "TH1.h"
#include "TH2D.h"
#include "TStyle.h"

#include "csv.h"

#include "SJSV_pcapreader.h"

#define CHN_PER_VMM 64
#define RECONSTRUCTION_LIST_LEN 10
#define RECONSTRUCTION_CHK_LEN 10000
#define MINIMUM_EVENT_HIT 5

INITIALIZE_EASYLOGGINGPP

class SJSV_eventbuilder
{
    public:
        struct parsed_frame {
            Int_t    uni_channel;
            Double_t      time_ns;
            Int_t    adc;
            Int_t    event_id = 0;
        };

        struct parsed_event {
            std::vector<parsed_frame*> frames_ptr;
            uint32_t id;
        };

        struct raw_mapping_info {
            std::vector<Short_t> board_num_array;
            std::vector<Short_t> channel_num_array;
            std::vector<Short_t> module_num_array;
            std::vector<Short_t> col_array;
            std::vector<Short_t> row_array;
            std::vector<Char_t>  gain_array;
        };

        struct channel_mapping_info {
            std::vector<Short_t> uni_channel_array;
            std::vector<Double_t> x_coords_array;
            std::vector<Double_t> y_coords_array;
            std::vector<Double_t> cell_size_array;
            std::vector<Bool_t> is_HG_array;
        };

        struct mapped_event {
            std::vector<Double_t> x_coords_array;
            std::vector<Double_t> y_coords_array;
            std::vector<Double_t> cell_size_array;
            std::vector<Double_t> value_array;
            std::vector<Double_t> value_LG_array;
            std::vector<Double_t> error_array;
        };
        
    public:
        SJSV_eventbuilder();
        ~SJSV_eventbuilder();

        inline channel_mapping_info* get_mapping_info_ptr() {
            return mapping_info_ptr;
        }

        inline int get_parsed_hit_number() {
            return vec_parsed_frame_ptr->size();
        }

        inline int get_parsed_event_number() {
            return vec_parsed_event_ptr->size();
        }

        inline parsed_frame* frame_at(uint64_t _index){
            if (_index >= vec_parsed_frame_ptr->size()) {
                LOG(ERROR) << "Index out of range";
                return nullptr;
            }
            return &vec_parsed_frame_ptr->at(_index);
        }

        // * Load raw data from rootfile created by SJSV_pcapreader
        // * @param _filename_str: filename of rootfile
        // * @return: true if success, false if failed
        bool load_raw_data(const std::string &_filename_str);

        // * Load mapping from csv file
        // * @param _filename_str: filename of csv file
        // * @return: vector of vector of the file
        raw_mapping_info read_mapping_csv_file(const std::string &_filename_str);

        channel_mapping_info generate_mapping_coordinates(
            const raw_mapping_info &_raw_mapping_info);

        bool load_mapping_file(const std::string &_filename_str);

        // * Map frames to coordinates
        // * @param _vec_parsed_frame: vector of parsed frames
        // * @param _mapping_info: mapping info
        // * @return: mapped_event
        // ! This function will ignore error information
        mapped_event map_event(const std::vector<SJSV_eventbuilder::parsed_frame> &_vec_parsed_frame, const SJSV_eventbuilder::channel_mapping_info &_mapping_info);
        mapped_event map_event(const parsed_event &_parsed_event, const SJSV_eventbuilder::channel_mapping_info &_mapping_info);

        bool reconstruct_event(Double_t _threshold_time_ns);

        // * Set cycle time of BCID in ns
        inline void set_bcid_cycle(uint8_t _bcid_cycle) {
            bcid_cycle = _bcid_cycle;
        }

        // * Set TDC slope time of TDC in ns
        inline void set_tdc_slope(uint8_t _tdc_slope) {
            tdc_slope = _tdc_slope;
        }

        // * Parse raw data to parsed data
        // * @return: true if success, false if failed
        bool parse_raw_data();

        // * Save parsed data to rootfile
        // * @param _filename_str: filename of rootfile
        // * @return: true if success, false if failed
        bool save_parsed_data(const std::string &_filename_str);

        // * Load parsed data from rootfile
        // * @param _filename_str: filename of rootfile
        // * @return: true if success, false if failed
        bool load_parsed_data(const std::string &_filename_str);

        std::vector<Double_t> get_event_adc_sum(bool _is_HG = true);
        bool is_frame_HG(const parsed_frame &_frame);
        std::vector<Double_t> get_frame_coord(const parsed_frame &_frame);

        // * Quick browse parsed data by channel
        // * @param _channel: channel to be browsed
        // * @param _start_time: start time in ns
        // * @param _end_time: end time in ns
        // * @return: TGraph of parsed data
        TGraph* quick_plot_single_channel(uint16_t _channel, double _start_time, double _end_time);
        
        // * Quick plot of time and frame index
        // * @param _start_time: start time in ns
        // * @param _end_time: end time in nss
        // * @return: TGraph of time and frame index
        TGraph* quick_plot_time_index(double _start_time, double _end_time);
        TGraph* quick_plot_time_index(void);

        // * Quick plot of multiple channels
        // * @param _vec_channel: vector of channels to be plotted
        // * @param _start_time: start time in ns
        // * @param _end_time: end time in ns
        // * @return: TMultiGraph of multiple channels
        TMultiGraph* quick_plot_multiple_channels(std::vector<uint16_t> _vec_channel, double _start_time, double _end_time);

        // * Quick histogram of single channel
        TH1D* quick_plot_single_channel_hist(uint16_t _channel, Int_t _bin_num, Double_t _bin_low, Double_t _bin_high);

        TH2D* quick_plot_mapped_event(const mapped_event &_mapped_event, Double_t _max_adc = -1);

        TH2D* plot_mapped_event_calib(const mapped_event &_mapped_event, const std::vector<Double_t> &_cell_id_vec, std::vector<Double_t> &_slope_vec, const std::vector<Double_t> &_intercept_vec, Double_t _max_adc = -1);

        TH2D* quick_plot_mapped_events_sum(void);

        TH2D* quick_plot_multiple_channels_hist(std::vector<uint16_t> _vec_channel, Int_t _bin_num, Double_t _bin_low, Double_t _bin_high);

        TH1D* quick_plot_event_chnnum_hist(int max_channel_num = 64);

        TH1D* quick_plot_event_adc_hist(Int_t _bin_num, Double_t _bin_low, Double_t _bin_high);

        TH1D* quick_plot_event_LG_adc_hist(Int_t _bin_num, Double_t _bin_low, Double_t _bin_high);

        // * Simple pedestal calculation - mean of lower 30% ADC
        std::vector<uint16_t> get_simple_pedestal();

        TH2D* quick_plot_mapped_events_sum2(void);

        bool reconstruct_event_list(Double_t _threshold_time_ns);

        // * Update pedestal to in-class vector
        // * after calling this function, single channel plot will subtract pedestal automatically
        inline void update_pedestal(const std::vector<uint16_t> &_pede_val) {
            if (_pede_val.size() == 0) {
                LOG(ERROR) << "Pedestal vector is empty" << std::endl;
                return;
            }

            if (is_pedestal_valid) {
                LOG(WARNING) << "Overwriting existing pedestal";
                is_pedestal_valid = false;
                vec_parsed_frame_ptr->clear();
            }
            
            this->vec_pedestal_ptr = new std::vector<uint16_t>(_pede_val);
            is_pedestal_valid = true;

            LOG(INFO) << "Pedestal updated";
            return;
        }

        std::vector<uint16_t>  load_pedestal_csv(const std::string &_filename_str);

        inline void enable_pedestal_subtraction(bool _enable = true) {
            if (_enable == pedestal_subtraction_enabled)
                LOG(WARNING) << "Pedestal subtraction is already " << (_enable ? "enabled" : "disabled") << std::endl;
            pedestal_subtraction_enabled = _enable;
        }

        inline parsed_event event_at(uint64_t _index) {
            if (_index >= vec_parsed_event_ptr->size()) {
                LOG(ERROR) << "Index out of range";
                return parsed_event();
            }
            return vec_parsed_event_ptr->at(_index);
        }

        inline void check_uni_channels(std::string _info){
            for (auto _frame: *vec_parsed_frame_ptr) {
                if (_frame.uni_channel > 20000) {
                    LOG(ERROR) << _info << " " << _frame.uni_channel << " " << _frame.time_ns << " " << _frame.adc;
                }
            }
        }

        inline void show_first_event_info() {
            if (vec_parsed_event_ptr->size() == 0) {
                LOG(ERROR) << "No event found";
                return;
            }
            auto _event = vec_parsed_event_ptr->at(0);
            // LOG(INFO) << "Event ID: " << _event.id;
            // LOG(INFO) << "Frame number: " << _event.frames_ptr.size();
            // LOG(INFO) << "Frame number (LG): " << _event.frames_LG_ptr.size();
            // LOG(INFO) << "Uni channes: ";
            // for (auto _frame: _event.frames_ptr) {
            //     LOG(INFO) << _frame->uni_channel;
            // }
            // LOG(INFO) << "ADC: ";
            // for (auto _frame: _event.frames_ptr) {
            //     LOG(INFO) << _frame->adc;
            // }
        }

    private:
        inline uint16_t get_uni_channel(const SJSV_pcapreader::uni_frame &_frame) {
            return _frame.vmm_id * 64  + _frame.channel;
        }

        inline uint32_t get_combined_corse_time(const SJSV_pcapreader::uni_frame &_frame) {
            return (_frame.offset << 12) + _frame.bcid;
        }

        inline double get_combined_corse_time_ns(const SJSV_pcapreader::uni_frame &_frame) {
            return double(get_combined_corse_time(_frame)) * double(bcid_cycle);
        }

        inline double get_combined_fine_time_ns(const SJSV_pcapreader::uni_frame &_frame) {
            return (get_combined_corse_time_ns(_frame) + 1.5 * double(bcid_cycle) - double(tdc_slope) * double(_frame.tdc) / 255.0);
        }

        // * Parse a frame
        // * @param _frame: frame to be parsed
        // * @param _offset_timestamp: timestamp difference from the first frame
        // * @return: parsed_frame
        parsed_frame parse_frame(const SJSV_pcapreader::uni_frame &_frame, uint64_t _offset_timestamp);
    
    private:
        bool is_raw_data_valid;
        bool is_parsed_data_valid;
        bool is_pedestal_valid;
        bool pedestal_subtraction_enabled;

        uint8_t bcid_cycle; // in ns
        uint8_t tdc_slope;  // in ns
        std::vector<SJSV_pcapreader::uni_frame>* vec_frame_ptr;
        std::vector<parsed_frame>* vec_parsed_frame_ptr;
        std::vector<uint16_t>* vec_pedestal_ptr;
        std::vector<parsed_event>* vec_parsed_event_ptr;

        channel_mapping_info* mapping_info_ptr;
};


