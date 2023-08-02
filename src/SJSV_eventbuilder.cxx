#include "SJSV_eventbuilder.h"

SJSV_eventbuilder::SJSV_eventbuilder():
    is_raw_data_valid(false),
    is_parsed_data_valid(false),
    bcid_cycle(25),
    tdc_slope(25) {
    vec_frame_ptr = new std::vector<SJSV_pcapreader::uni_frame>;
}

SJSV_eventbuilder::~SJSV_eventbuilder() {
    if (vec_frame_ptr != nullptr) {
        delete vec_frame_ptr;
    }
}

bool SJSV_eventbuilder::load_raw_data(const std::string &_filename_str){
    if (_filename_str.empty()) {
        LOG(ERROR) << "Filename is empty";
        return false;
    }
    
    TFile *rootfile = new TFile(_filename_str.c_str(), "READ");
    if (rootfile->IsZombie()) {
        LOG(ERROR) << "Cannot open rootfile: " << _filename_str;
        return false;
    }
    TTree *tree = (TTree*)rootfile->Get("tree");
    if (tree == nullptr) {
        LOG(ERROR) << "Cannot find tree in rootfile: " << _filename_str;
        return false;
    }

    uint8_t  _offset;
    uint8_t  _vmm_id;
    uint16_t _adc;
    uint16_t _bcid;
    bool     _daqdata38;
    uint8_t  _channel;
    uint8_t  _tdc;
    uint64_t _timestamp;
    bool    _flag_daq;

    tree->SetBranchAddress("offset", &_offset);
    tree->SetBranchAddress("vmm_id", &_vmm_id);
    tree->SetBranchAddress("adc", &_adc);
    tree->SetBranchAddress("bcid", &_bcid);
    tree->SetBranchAddress("daqdata38", &_daqdata38);
    tree->SetBranchAddress("channel", &_channel);
    tree->SetBranchAddress("tdc", &_tdc);
    tree->SetBranchAddress("timestamp", &_timestamp);
    tree->SetBranchAddress("flag_daq", &_flag_daq);

    int64_t nentries = tree->GetEntries();

    for (int64_t ientry = 0; ientry < nentries; ientry++) {
        tree->GetEntry(ientry);
        SJSV_pcapreader::uni_frame _frame;
        _frame.offset = _offset;
        _frame.vmm_id = _vmm_id;
        _frame.adc = _adc;
        _frame.bcid = _bcid;
        _frame.daqdata38 = _daqdata38;
        _frame.channel = _channel;
        _frame.tdc = _tdc;
        _frame.timestamp = _timestamp;
        _frame.flag_daq = _flag_daq;
        vec_frame_ptr->push_back(_frame);
    }

    rootfile->Close();
    LOG(INFO) << "Loaded " << nentries << " entries from " << _filename_str;
    is_raw_data_valid = true;
    return true;
}

SJSV_eventbuilder::parsed_frame SJSV_eventbuilder::parse_frame(const SJSV_pcapreader::uni_frame &_frame, uint64_t _offset_timestamp) {
    parsed_frame _parsed_frame;
    if (!_frame.flag_daq) {
        LOG(WARNING) << "DAQ flag is not set, not a DAQ frame";
        return _parsed_frame;
    }
    _parsed_frame.uni_channel = get_uni_channel(_frame);
    _parsed_frame.time_ns = get_combined_fine_time_ns(_frame) + double(_offset_timestamp) * double(bcid_cycle);
    _parsed_frame.adc = _frame.adc;
    return _parsed_frame;
}

bool SJSV_eventbuilder::parse_raw_data(){
    if (!is_raw_data_valid) {
        LOG(ERROR) << "Raw data is not valid for parsing";
        return false;
    }

    if (vec_frame_ptr->empty()) {
        LOG(ERROR) << "Raw data is empty";
        return false;
    }

    if (is_parsed_data_valid) {
        LOG(INFO) << "Parsed data is valid, deleting old data";
        vec_parsed_frame_ptr->clear();
    }

    if (vec_parsed_frame_ptr != nullptr) {
        LOG(INFO) << "Parsed data is not empty, deleting old data";
        delete vec_parsed_frame_ptr;
    }

    vec_parsed_frame_ptr = new std::vector<parsed_frame>;

    auto _frame_num = vec_frame_ptr->size();
    uint64_t _timestamp_start = 0;
    uint64_t _timestamp_current = 0;
    bool _first_timestamp_found = false;
    uint32_t _skipped_daq_frame_count = 0;
    uint32_t _time_frame_count = 0;

    for (auto i=0; i< _frame_num; i++) {
        auto _frame = vec_frame_ptr->at(i);
        if (_frame.flag_daq == 1) {
            if (!_first_timestamp_found) {
                _skipped_daq_frame_count++;
                continue;
            }
            auto _timestamp_offset = _timestamp_current - _timestamp_start;
            auto _parsed_frame = parse_frame(_frame, _timestamp_offset);
            vec_parsed_frame_ptr->push_back(_parsed_frame);
        } else {
            _time_frame_count++;
            if (!_first_timestamp_found) {
                _timestamp_start = _frame.timestamp;
                _first_timestamp_found = true;
            }
            _timestamp_current = _frame.timestamp;
        }
    }

    LOG(INFO) << vec_parsed_frame_ptr->size() << " frames parsed";
    LOG(INFO) << _skipped_daq_frame_count << " DAQ frames skipped";
    LOG(INFO) << _time_frame_count << " time frames found";

    is_parsed_data_valid = true;
    return true;
}

bool SJSV_eventbuilder::save_parsed_data(const std::string &_filename_str) {
    if (!is_parsed_data_valid) {
        LOG(ERROR) << "Parsed data is not valid for saving";
        return false;
    }

    TFile *rootfile = new TFile(_filename_str.c_str(), "RECREATE");
    if (rootfile->IsZombie()) {
        LOG(ERROR) << "Cannot open rootfile: " << _filename_str;
        return false;
    }

    TTree *tree = new TTree("tree", "tree");

    uint16_t uni_channel;
    double time_ns;
    uint16_t adc;

    tree->Branch("uni_channel", &uni_channel, "uni_channel/I");
    tree->Branch("time_ns", &time_ns, "time_ns/D");
    tree->Branch("adc", &adc, "adc/I");

    for (auto i=0; i<vec_parsed_frame_ptr->size(); i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        uni_channel = _parsed_frame.uni_channel;
        time_ns = _parsed_frame.time_ns;
        adc = _parsed_frame.adc;
        tree->Fill();
    }

    rootfile->Write();
    rootfile->Close();

    return true;
}