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

#include "csv.h"

#include "SJSV_pcapreader.h"

#define CHN_PER_VMM 64

INITIALIZE_EASYLOGGINGPP

class SJSV_eventbuilder
{
    public:
        struct parsed_frame {
            uint16_t    uni_channel;
            double      time_ns;
            uint16_t    adc;
        };

        struct raw_mapping_info {
            std::vector<Short_t> board_num_array;
            std::vector<Short_t> channel_num_array;
            std::vector<Short_t> module_num_array;
            std::vector<Short_t> col_array;
            std::vector<Short_t> row_array;
        };

        struct channel_mapping_info {
            std::vector<Short_t> uni_channel_array;
            std::vector<Double_t> x_coords_array;
            std::vector<Double_t> y_coords_array;
            std::vector<Double_t> cell_size_array;
        };
        
    public:
        SJSV_eventbuilder();
        ~SJSV_eventbuilder();

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

        // * Quick plot of multiple channels
        // * @param _vec_channel: vector of channels to be plotted
        // * @param _start_time: start time in ns
        // * @param _end_time: end time in ns
        // * @return: TMultiGraph of multiple channels
        TMultiGraph* quick_plot_multiple_channels(std::vector<uint16_t> _vec_channel, double _start_time, double _end_time);

        // * Quick histogram of single channel
        TH1D* quick_plot_single_channel_hist(uint16_t _channel, Int_t _bin_num, Double_t _bin_low, Double_t _bin_high);

        // * Simple pedestal calculation - mean of lower 30% ADC
        std::vector<uint16_t> get_simple_pedestal();

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

    private:
        inline uint16_t get_uni_channel(const SJSV_pcapreader::uni_frame &_frame) {
            return (_frame.vmm_id << 6) + _frame.channel;
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

        channel_mapping_info* mapping_info_ptr;
};


