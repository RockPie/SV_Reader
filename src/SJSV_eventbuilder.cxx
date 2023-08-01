#include "SJSV_eventbuilder.h"

SJSV_eventbuilder::SJSV_eventbuilder():
    is_raw_data_valid(false),
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