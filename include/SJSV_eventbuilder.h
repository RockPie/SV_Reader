#pragma once

#include "easylogging++.h"

#include "TFile.h"
#include "TTree.h"

#include "SJSV_pcapreader.h"

INITIALIZE_EASYLOGGINGPP

class SJSV_eventbuilder
{
    public:
        struct parsed_frame {
            uint16_t    uni_channel;
            double      time_ns;
            uint16_t    adc;
        };
        
    public:
        SJSV_eventbuilder();
        ~SJSV_eventbuilder();

        // * Load raw data from rootfile created by SJSV_pcapreader
        // * @param _filename_str: filename of rootfile
        // * @return: true if success, false if failed
        bool load_raw_data(const std::string &_filename_str);

        // * Set cycle time of BCID in ns
        inline void set_bcid_cycle(uint8_t _bcid_cycle) {
            bcid_cycle = _bcid_cycle;
        }

        // * Set TDC slope time of TDC in ns
        inline void set_tdc_slope(uint8_t _tdc_slope) {
            tdc_slope = _tdc_slope;
        }

        bool parse_raw_data();

        bool save_parsed_data(const std::string &_filename_str);

    private:
        inline uint16_t get_uni_channel(const SJSV_pcapreader::uni_frame &_frame) {
            return (_frame.vmm_id << 6) + _frame.channel;
        }

        inline uint32_t get_combined_corse_time(const SJSV_pcapreader::uni_frame &_frame) {
            return (_frame.offset << 12) + _frame.bcid;
        }

        // todo: check if 32 bit is enough
        inline double get_combined_corse_time_ns(const SJSV_pcapreader::uni_frame &_frame) {
            return double(get_combined_corse_time(_frame)) * double(bcid_cycle);
        }

        inline double get_combined_fine_time_ns(const SJSV_pcapreader::uni_frame &_frame) {
            return (get_combined_corse_time_ns(_frame) + 1.5 * double(bcid_cycle) - double(tdc_slope) * double(_frame.tdc) / 255.0);
        }

        parsed_frame parse_frame(const SJSV_pcapreader::uni_frame &_frame, uint64_t _offset_timestamp);
    
    private:
        bool is_raw_data_valid;
        bool is_parsed_data_valid;

        uint8_t bcid_cycle; // in ns
        uint8_t tdc_slope;  // in ns
        std::vector<SJSV_pcapreader::uni_frame>* vec_frame_ptr;
        std::vector<parsed_frame>* vec_parsed_frame_ptr;
};


