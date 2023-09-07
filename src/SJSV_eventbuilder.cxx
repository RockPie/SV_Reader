#include "SJSV_eventbuilder.h"

SJSV_eventbuilder::SJSV_eventbuilder():
    is_raw_data_valid(false),
    is_parsed_data_valid(false),
    is_pedestal_valid(false),
    pedestal_subtraction_enabled(false),
    bcid_cycle(25),
    tdc_slope(25) {
    vec_frame_ptr = new std::vector<SJSV_pcapreader::uni_frame>;
}

SJSV_eventbuilder::~SJSV_eventbuilder() {
    if (vec_frame_ptr != nullptr) {
        delete vec_frame_ptr;
    }
    if (vec_parsed_frame_ptr != nullptr) {
        delete vec_parsed_frame_ptr;
    }
    if (vec_pedestal_ptr != nullptr) {
        delete vec_pedestal_ptr;
    }
    if (mapping_info_ptr != nullptr) {
        delete mapping_info_ptr;
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

    if (is_parsed_data_valid) {
        vec_frame_ptr->clear();
        is_parsed_data_valid = false;
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

TGraph* SJSV_eventbuilder::quick_plot_single_channel(uint16_t _channel, double _start_time, double _end_time){
    if (!is_parsed_data_valid) {
        LOG(ERROR) << "Parsed data is not valid for browsing";
        return nullptr;
    }

    if (vec_parsed_frame_ptr->empty()) {
        LOG(ERROR) << "Parsed data is empty";
        return nullptr;
    }

    auto _graph = new TGraph();
    auto _graph_name = "graph_ch" + std::to_string(_channel);
    _graph->SetTitle(_graph_name.c_str());
    _graph->SetName(_graph_name.c_str());

    auto _frame_num = vec_parsed_frame_ptr->size();
    uint32_t _plot_point_cnt = 0;

    for (auto i=0; i<_frame_num; i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        if (_parsed_frame.uni_channel == _channel) {
            if (_parsed_frame.time_ns >= _start_time && _parsed_frame.time_ns <= _end_time) {
                auto _adc_buffer = _parsed_frame.adc;
                if (pedestal_subtraction_enabled) {
                    if (!is_pedestal_valid) {
                        LOG(WARNING) << "Pedestal is not valid for browsing";
                    } else {
                        if (_adc_buffer >= vec_pedestal_ptr->at(_channel))
                            _adc_buffer -= vec_pedestal_ptr->at(_channel);
                        else
                            _adc_buffer = 0;
                    }
                    
                }
                _graph->SetPoint(_graph->GetN(), _parsed_frame.time_ns, _adc_buffer);
                _plot_point_cnt++;
            }
        }
    }

    // set x axis label
    auto _xaxis = _graph->GetXaxis();
    _xaxis->SetTitle("Time (ns)");

    // set y axis label
    auto _yaxis = _graph->GetYaxis();
    _yaxis->SetTitle("ADC");

    if (_plot_point_cnt == 0) {
        LOG(WARNING) << "No points plotted";
        delete _graph;
        return nullptr;
    } else {
        LOG(INFO) << _plot_point_cnt << " points plotted";
    }


    return _graph;
}

TGraph* SJSV_eventbuilder::quick_plot_time_index(double _start_time, double _end_time){
    if (!is_parsed_data_valid) {
        LOG(ERROR) << "Parsed data is not valid for browsing";
        return nullptr;
    }

    if (vec_parsed_frame_ptr->empty()) {
        LOG(ERROR) << "Parsed data is empty";
        return nullptr;
    }

    // LOG(DEBUG) << "Plotting time index";

    auto _graph = new TGraph();
    auto _graph_name = "reconstucted time vs frame index";

    _graph->SetTitle(_graph_name);
    _graph->SetName(_graph_name);
    // add grid
    _graph->SetLineWidth(2);

    auto _frame_num = vec_parsed_frame_ptr->size();
    uint32_t _plot_point_cnt = 0;

    for (auto i=0; i<_frame_num; i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        if (_parsed_frame.time_ns >= _start_time && _parsed_frame.time_ns <= _end_time) {
            _plot_point_cnt++;
            // LOG(DEBUG) << "Plotting frame " << i << " at time " << _parsed_frame.time_ns;
            _graph->SetPoint(_graph->GetN(), i, _parsed_frame.time_ns);
        }
    }

    // set x axis label
    auto _xaxis = _graph->GetXaxis();
    _xaxis->SetTitle("Frame index");

    // set y axis label
    auto _yaxis = _graph->GetYaxis();
    _yaxis->SetTitle("Time (ns)");

    if (_plot_point_cnt == 0) {
        LOG(WARNING) << "No points plotted";
        delete _graph;
        return nullptr;
    } else {
        LOG(INFO) << _plot_point_cnt << " points plotted";
    }

    return _graph;
}

TMultiGraph* SJSV_eventbuilder::quick_plot_multiple_channels(std::vector<uint16_t> _vec_channel, double _start_time, double _end_time) {
    if (_vec_channel.empty()) {
        LOG(ERROR) << "Channel vector is empty";
        return nullptr;
    }

    auto _mg = new TMultiGraph();
    auto _mg_name = "multichannel ADCs";
    _mg->SetTitle(_mg_name);

    // auto _xaxis = _mg->GetXaxis();
    // _xaxis->SetTitle("Time (ns)");

    // auto _yaxis = _mg->GetYaxis();
    // _yaxis->SetTitle("ADC");

    for (auto i=0; i<_vec_channel.size(); i++) {
        auto _channel = _vec_channel.at(i);
        auto _graph = quick_plot_single_channel(_channel, _start_time, _end_time);
        
        if (_graph == nullptr) {
            LOG(WARNING) << "Graph for channel " << _channel << " is null";
            auto _graph_placeholder = new TGraph();
            _mg->Add(_graph_placeholder);
            continue;
        }
        _graph->SetLineColor(i+1);
        _graph->SetLineWidth(3);    
        _mg->Add(_graph);
    }
    auto _xaxis = _mg->GetXaxis();
    _xaxis->SetTitle("Time (ns)");

    auto _yaxis = _mg->GetYaxis();
    _yaxis->SetTitle("ADC");

    return _mg;
}

TH1D* SJSV_eventbuilder::quick_plot_single_channel_hist(uint16_t _channel, Int_t _bin_num, Double_t _bin_low, Double_t _bin_high) {
    if (!is_parsed_data_valid) {
        LOG(ERROR) << "Parsed data is not valid for browsing";
        return nullptr;
    }

    if (vec_parsed_frame_ptr->empty()) {
        LOG(ERROR) << "Parsed data is empty";
        return nullptr;
    }

    auto _hist = new TH1D();
    auto _hist_name = "hist_ch" + std::to_string(_channel);
    _hist->SetTitle(_hist_name.c_str());
    _hist->SetName(_hist_name.c_str());

    // set bin number
    _hist->SetBins(_bin_num, _bin_low, _bin_high);

    auto _frame_num = vec_parsed_frame_ptr->size();
    uint32_t _plot_point_cnt = 0;

    for (auto i=0; i<_frame_num; i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        if (_parsed_frame.uni_channel == _channel) {
            _hist->Fill(_parsed_frame.adc);
            _plot_point_cnt++;
        }
    }

    // normalize
    // _hist->Scale(1.0/_hist->Integral());

    if (_plot_point_cnt == 0) {
        LOG(WARNING) << "No points plotted";
        delete _hist;
        return nullptr;
    } else {
        LOG(INFO) << _plot_point_cnt << " points plotted";
    }

    return _hist;
}

std::vector<uint16_t> SJSV_eventbuilder::get_simple_pedestal() {
    std::vector<uint16_t> _vec_pedestal;
    if (!is_parsed_data_valid) {
        LOG(ERROR) << "Parsed data is not valid for browsing";
        return _vec_pedestal;
    }
    if (vec_parsed_frame_ptr->empty()) {
        LOG(ERROR) << "Parsed data is empty";
        return _vec_pedestal;
    }
    std::vector<std::vector<uint16_t>> _channel_adc_values;
    auto _frame_num = vec_parsed_frame_ptr->size();
    for (auto i=0; i<_frame_num; i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        auto _channel = _parsed_frame.uni_channel;
        auto _adc = _parsed_frame.adc;
        if (_channel_adc_values.size() < _channel+1) {
            _channel_adc_values.resize(_channel+1);
        }
        _channel_adc_values.at(_channel).push_back(_adc);
    }

    for (auto _single_channel_adc_values : _channel_adc_values) {
        if (_single_channel_adc_values.size() <= 4) {
            LOG(WARNING) << "Channel has too few adc values";
            _vec_pedestal.push_back(0);
            continue;
        }
        // get the average of lower 30% adc values
        std::sort(_single_channel_adc_values.begin(), _single_channel_adc_values.end());
        auto _pedestal_value = 0;
        for (auto i=0; i<_single_channel_adc_values.size()*0.3; i++) {
            _pedestal_value += _single_channel_adc_values.at(i);
        }
        _pedestal_value /= _single_channel_adc_values.size()*0.3;
        _vec_pedestal.push_back(_pedestal_value);
    }

    return _vec_pedestal;
}

std::vector<uint16_t> SJSV_eventbuilder::load_pedestal_csv(const std::string &_filename_str) {
    if (_filename_str.empty()) {
        LOG(ERROR) << "Filename is empty";
        return std::vector<uint16_t>();
    }

    io::CSVReader<5> _in(_filename_str.c_str());
    _in.read_header(io::ignore_extra_column, "hybrid_id", "fec", "vmm", "ch", "pedestal [mV]");
    std::string _hybrid_id_str;
    std::string _fec;
    std::string _vmm;
    std::string _ch;
    std::string _pedestal;

    std::vector<uint16_t> _vec_pedestal;

    while (_in.read_row(_hybrid_id_str, _fec, _vmm, _ch, _pedestal)) {
        uint16_t _uni_chn = 0;
        uint16_t _pedestal_value = 0;
        try
        {
            _uni_chn = std::stoi(_ch) + std::stoi(_vmm)*64;
            _pedestal_value = std::stoi(_pedestal);
        }
        catch(const std::exception& e)
        {
            // ! this can be caused by additional header in the csv file
            LOG(WARNING) << "Exception caught when converting channel number to integer: " << e.what();
            continue;
        }
        
        if (_vec_pedestal.size() < _uni_chn+1) {
            _vec_pedestal.resize(_uni_chn+1);
        }
        _vec_pedestal.at(_uni_chn) = _pedestal_value;
    }

    LOG(INFO) << "Pedestal loaded from " << _filename_str << " with " << _vec_pedestal.size() << " channels";

    return _vec_pedestal;
}

SJSV_eventbuilder::raw_mapping_info SJSV_eventbuilder::read_mapping_csv_file(const  std::string &_filename_str){
    LOG(INFO) << "Reading mapping file: " << _filename_str;
    auto _res = SJSV_eventbuilder::raw_mapping_info();

    if (!std::filesystem::exists(_filename_str)) {
        LOG(ERROR) << "File not found: " << _filename_str;
        return _res;
    }

    std::vector<std::vector<Short_t>> _mapping_array_res;
    io::CSVReader <5> in(_filename_str.c_str());
    in.read_header(io::ignore_extra_column, "BoardNum", "ChannelNum", "ModuleNum", "Col", "Row");
    Short_t _board_num, _channel_num, _module_num, _col, _row;
    std::vector<Short_t> _board_num_array, _channel_num_array, _module_num_array, _col_array, _row_array;
    while (in.read_row(_board_num, _channel_num, _module_num, _col, _row)) {
        _res.board_num_array.push_back(_board_num);
        _res.channel_num_array.push_back(_channel_num);
        _res.module_num_array.push_back(_module_num);
        _res.col_array.push_back(_col);
        _res.row_array.push_back(_row);
    }
    return _res;
}

// * Assuming the central module has 7x7 of 5x5 pads
// * Others are 5x5 of 7x7 pads
SJSV_eventbuilder::channel_mapping_info SJSV_eventbuilder::generate_mapping_coordinates(
    const raw_mapping_info &_raw_mapping_info){

    auto _res = SJSV_eventbuilder::channel_mapping_info();
    
    auto _array_size = _raw_mapping_info.board_num_array.size();
    if (_array_size == 0){
        LOG(ERROR) << "Mapping array size is 0";
        return _res;
    }
    if (_array_size != _raw_mapping_info.channel_num_array.size() ||
        _array_size != _raw_mapping_info.module_num_array.size() ||
        _array_size != _raw_mapping_info.col_array.size() ||
        _array_size != _raw_mapping_info.row_array.size()) {
        LOG(ERROR) << "Mapping array size not match";
        return _res;
    }

    // * Step 1. generate one-dimensional channel num
    for (auto i = 0; i < _array_size; i++)
        _res.uni_channel_array.push_back(_raw_mapping_info.board_num_array.at(i) * 64 + _raw_mapping_info.channel_num_array.at(i));

    // * Step 2. generate x and y coordinate
    for (auto i = 0; i < _array_size; i++) {
        Double_t _x_base = 0;
        Double_t _y_base = 0;
        switch (_raw_mapping_info.module_num_array.at(i))
        {
        case 0:{
            _x_base = 3;
            _y_base = 101;
            break;
        }
        case 1:{
            _x_base = 38;
            _y_base = 101;
            break;
        }
        case 2:{
            _x_base = 73;
            _y_base = 101;
            break;
        }
        case 3:{
            _x_base = 3;
            _y_base = 66;
            break;
        }
        case 4:{
            _x_base = 37;
            _y_base = 67;
            break;
        }
        case 5:{
            _x_base = 73;
            _y_base = 66;
            break;
        }
        case 6:{
            _x_base = 3;
            _y_base = 31;
            break;
        }
        case 7:{
            _x_base = 38;
            _y_base = 31;
            break;
        }
        case 8:{
            _x_base = 73;
            _y_base = 31;
            break;
        }
        default:
            LOG(ERROR) << "Mapping module number error: " << _raw_mapping_info.module_num_array.at(i);
            break;
        }
        // * Central module
        if (_raw_mapping_info.module_num_array.at(i) == 4){
            _res.x_coords_array.push_back(_x_base + _raw_mapping_info.col_array.at(i) * 5);
            _res.y_coords_array.push_back(_y_base - _raw_mapping_info.row_array.at(i) * 5);
            _res.cell_size_array.push_back(5);
        }
        // * Other modules
        else {
            _res.x_coords_array.push_back(_x_base + _raw_mapping_info.col_array.at(i) * 7);
            _res.y_coords_array.push_back(_y_base - _raw_mapping_info.row_array.at(i) * 7);
            _res.cell_size_array.push_back(7);
        }
    }
    return _res;
}

bool SJSV_eventbuilder::load_mapping_file(const std::string &_filename_str){
    auto _raw_mapping_info = this->read_mapping_csv_file(_filename_str);
    auto _channel_mapping_info = this->generate_mapping_coordinates(_raw_mapping_info);
    if (_channel_mapping_info.uni_channel_array.empty()) {
        LOG(ERROR) << "Channel mapping info is empty";
        return false;
    }
    this->mapping_info_ptr = new channel_mapping_info(_channel_mapping_info);
    LOG(INFO) << "Loaded mapping file: " << _filename_str;
    return true;
}