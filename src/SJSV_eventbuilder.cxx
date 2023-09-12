#include "SJSV_eventbuilder.h"

SJSV_eventbuilder::SJSV_eventbuilder():
    is_raw_data_valid(false),
    is_parsed_data_valid(false),
    is_pedestal_valid(false),
    pedestal_subtraction_enabled(false),
    bcid_cycle(25),
    tdc_slope(25) {
    vec_frame_ptr = new std::vector<SJSV_pcapreader::uni_frame>;
    vec_parsed_frame_ptr = new std::vector<parsed_frame>;
    vec_pedestal_ptr = new std::vector<uint16_t>;
    mapping_info_ptr = new channel_mapping_info;
    vec_parsed_event_ptr = new std::vector<parsed_event>;
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
    if (vec_parsed_event_ptr != nullptr) {
        delete vec_parsed_event_ptr;
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
            auto _parsed_frame_new = new parsed_frame(_parsed_frame);
            vec_parsed_frame_ptr->push_back(*_parsed_frame_new);
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

    Int_t uni_channel;
    Double_t time_ns;
    Int_t adc;
    Int_t event_id;

    tree->Branch("uni_channel", &uni_channel, "uni_channel/I");
    tree->Branch("time_ns", &time_ns, "time_ns/D");
    tree->Branch("adc", &adc, "adc/I");
    tree->Branch("event_id", &event_id, "event_id/I");

    for (auto i=0; i<vec_parsed_frame_ptr->size(); i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        uni_channel = _parsed_frame.uni_channel;
        time_ns = _parsed_frame.time_ns;
        adc = _parsed_frame.adc;
        event_id = _parsed_frame.event_id;
        tree->Fill();
    }

    rootfile->Write();
    rootfile->Close();

    return true;
}

bool SJSV_eventbuilder::load_parsed_data(const std::string &_filename_str){
    if(_filename_str.empty()) {
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
        vec_parsed_frame_ptr->clear();
        is_parsed_data_valid = false;
    }

    Int_t uni_channel;
    Double_t time_ns;
    Int_t adc;
    Int_t event_id;

    tree->SetBranchAddress("uni_channel", &uni_channel);
    tree->SetBranchAddress("time_ns", &time_ns);
    tree->SetBranchAddress("adc", &adc);
    tree->SetBranchAddress("event_id", &event_id);

    int64_t nentries = tree->GetEntries();
    for (int64_t ientry = 0; ientry < nentries; ientry++) {
        tree->GetEntry(ientry);
        parsed_frame _parsed_frame;
        _parsed_frame.uni_channel = uni_channel;
        _parsed_frame.time_ns = time_ns;
        _parsed_frame.adc = adc;
        _parsed_frame.event_id = event_id;
        vec_parsed_frame_ptr->push_back(_parsed_frame);
    }

    LOG(INFO) << "Loaded " << nentries << " entries from " << _filename_str;

    rootfile->Close();
    is_parsed_data_valid = true;
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
    _graph->SetStats(1);
    if (_plot_point_cnt == 0) {
        // LOG(WARNING) << "No points plotted";
        delete _graph;
        return nullptr;
    } else {
        //LOG(INFO) << _plot_point_cnt << " points plotted";
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
        //LOG(WARNING) << "No points plotted";
        delete _graph;
        return nullptr;
    } else {
        LOG(INFO) << _plot_point_cnt << " points plotted";
    }

    return _graph;
}

TGraph* SJSV_eventbuilder::quick_plot_time_index(void){
    double _global_min_time = 0;
    double _global_max_time = 0;
    if (!is_parsed_data_valid) {
        LOG(ERROR) << "Parsed data is not valid for browsing";
        return nullptr;
    }
    for (auto _parsed_frame : *vec_parsed_frame_ptr) {
        if (_parsed_frame.time_ns < _global_min_time) {
            _global_min_time = _parsed_frame.time_ns;
        }
        if (_parsed_frame.time_ns > _global_max_time) {
            _global_max_time = _parsed_frame.time_ns;
        }
    }
    return quick_plot_time_index(_global_min_time, _global_max_time);
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
            // LOG(WARNING) << "Graph for channel " << _channel << " is null";
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
        //LOG(WARNING) << "No points plotted";
        delete _hist;
        return nullptr;
    } else {
        // LOG(INFO) << _plot_point_cnt << " points plotted";
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

    for (auto _channel_index=0; _channel_index<_channel_adc_values.size(); _channel_index++) {
        auto _single_channel_adc_values = _channel_adc_values.at(_channel_index);
        if (_single_channel_adc_values.size() <= 4) {
            LOG(WARNING) << "chn" << _channel_index << " too few adc values: ";
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

bool SJSV_eventbuilder::is_frame_HG(const parsed_frame &_frame){
    auto _uni_channel = _frame.uni_channel;
    auto _vec_uni_channel = mapping_info_ptr->uni_channel_array;
    auto _vec_Gain = mapping_info_ptr->is_HG_array;
    auto _vec_index = std::find(_vec_uni_channel.begin(), _vec_uni_channel.end(), _uni_channel);
    if (_vec_index == _vec_uni_channel.end()) {
        // LOG(ERROR) << "Cannot find uni channel " << _uni_channel << " in mapping info";
        return false;
    }
    auto _index = std::distance(_vec_uni_channel.begin(), _vec_index);
    return _vec_Gain.at(_index);
}

std::vector<Double_t> SJSV_eventbuilder::get_frame_coord(const parsed_frame &_frame){
    std::vector<Double_t> _frame_coord = {-1, -1};
    auto _uni_channel = _frame.uni_channel;
    auto _vec_uni_channel = mapping_info_ptr->uni_channel_array;
    auto _vec_x_coord = mapping_info_ptr->x_coords_array;
    auto _vec_y_coord = mapping_info_ptr->y_coords_array;
    auto _vec_index = std::find(_vec_uni_channel.begin(), _vec_uni_channel.end(), _uni_channel);
    if (_vec_index == _vec_uni_channel.end()) {
        // LOG(ERROR) << "Cannot find uni channel " << _uni_channel << " in mapping info";
        return _frame_coord;
    }
    auto _index = std::distance(_vec_uni_channel.begin(), _vec_index);
    _frame_coord.at(0) = _vec_x_coord.at(_index);
    _frame_coord.at(1) = _vec_y_coord.at(_index);
    return _frame_coord;
}


std::vector<Double_t> SJSV_eventbuilder::get_event_adc_sum(bool _is_HG){
    std::vector<Double_t> _vec_event_adc_sum;
    if (_is_HG){
        for (auto _event: *vec_parsed_event_ptr) {
            Double_t _adc_sum = 0;
            for (auto _frame_ptr : _event.frames_ptr) {
                if (is_frame_HG(*_frame_ptr))
                    _adc_sum += _frame_ptr->adc;
            }
            _vec_event_adc_sum.push_back(_adc_sum);
        }
    } else {
        for (auto _event: *vec_parsed_event_ptr) {
            Double_t _adc_sum = 0;
            for (auto _frame_ptr : _event.frames_ptr) {
                if (!is_frame_HG(*_frame_ptr))
                    _adc_sum += _frame_ptr->adc;
            }
            _vec_event_adc_sum.push_back(_adc_sum);
        }
    }
    return _vec_event_adc_sum;
}


SJSV_eventbuilder::raw_mapping_info SJSV_eventbuilder::read_mapping_csv_file(const  std::string &_filename_str){
    LOG(INFO) << "Reading mapping file: " << _filename_str;
    auto _res = SJSV_eventbuilder::raw_mapping_info();

    if (_filename_str.empty()) {
        LOG(ERROR) << "File not found: " << _filename_str;
        return _res;
    }

    std::vector<std::vector<Short_t>> _mapping_array_res;
    io::CSVReader <6> in(_filename_str.c_str());
    in.read_header(io::ignore_extra_column, "BoardNum", "ChannelNum", "ModuleNum", "Col", "Row", "Gain");
    Short_t _board_num, _channel_num, _module_num, _col, _row;
    Char_t _gain;
    std::vector<Short_t> _board_num_array, _channel_num_array, _module_num_array, _col_array, _row_array;
    std::vector<Char_t> _gain_array;
    while (in.read_row(_board_num, _channel_num, _module_num, _col, _row, _gain)) {
        _res.board_num_array.push_back(_board_num);
        _res.channel_num_array.push_back(_channel_num);
        _res.module_num_array.push_back(_module_num);
        _res.col_array.push_back(_col);
        _res.row_array.push_back(_row);
        _res.gain_array.push_back(_gain);
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
        _array_size != _raw_mapping_info.row_array.size() ||
        _array_size != _raw_mapping_info.gain_array.size()) {
        LOG(ERROR) << "Mapping array size not match";
        return _res;
    }

    // * Step 1. generate one-dimensional channel num
    for (auto i = 0; i < _array_size; i++)
        _res.uni_channel_array.push_back(_raw_mapping_info.board_num_array.at(i) * 64 + _raw_mapping_info.channel_num_array.at(i));

    for (auto i = 0; i < _array_size; i++)
        _res.is_HG_array.push_back(_raw_mapping_info.gain_array.at(i) == 'H');

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

SJSV_eventbuilder::mapped_event SJSV_eventbuilder::map_event(const std::vector<SJSV_eventbuilder::parsed_frame> &_vec_parsed_frame, const SJSV_eventbuilder::channel_mapping_info &_mapping_info){
    auto _res = SJSV_eventbuilder::mapped_event();
    if (_vec_parsed_frame.empty()) {
        LOG(ERROR) << "Parsed frame vector is empty";
        return _res;
    }
    if (_mapping_info.uni_channel_array.empty()) {
        LOG(ERROR) << "Mapping info is empty";
        return _res;
    }

    auto _vec_uni_channel = _mapping_info.uni_channel_array;
    auto _vec_x_coords  = _mapping_info.x_coords_array;
    auto _vec_y_coords  = _mapping_info.y_coords_array;
    auto _vec_cell_size = _mapping_info.cell_size_array;
    auto _vec_Gain = _mapping_info.is_HG_array;

    for (auto i=0; i<_vec_parsed_frame.size(); i++) {
        auto _parsed_frame = _vec_parsed_frame.at(i);
        auto _uni_channel = _parsed_frame.uni_channel;
        auto _adc = _parsed_frame.adc;

        int _index = std::find(_vec_uni_channel.begin(), _vec_uni_channel.end(), _uni_channel) - _vec_uni_channel.begin();

        if (_index == _vec_uni_channel.size()) {
            //LOG(ERROR) << "Cannot find channel " << _uni_channel << " in mapping info";
            continue;
        }

        auto _x_coord = _vec_x_coords.at(_index);
        auto _y_coord = _vec_y_coords.at(_index);
        auto _cell_size = _vec_cell_size.at(_index);

        _res.x_coords_array.push_back(_x_coord);
        _res.y_coords_array.push_back(_y_coord);
        _res.cell_size_array.push_back(_cell_size);
        if (_vec_Gain.at(_index)){
            _res.value_array.push_back(_adc);
            _res.value_LG_array.push_back(-1);
        } else {
            _res.value_array.push_back(-1);
            _res.value_LG_array.push_back(_adc);
        }
    }

    return _res;
}

SJSV_eventbuilder::mapped_event SJSV_eventbuilder::map_event(const SJSV_eventbuilder::parsed_event &_parsed_event, const SJSV_eventbuilder::channel_mapping_info &_mapping_info){
    std::vector<SJSV_eventbuilder::parsed_frame> _adapter_vec_parsed_frame;
    for (auto _parsed_frame : _parsed_event.frames_ptr) {
        _adapter_vec_parsed_frame.push_back(*_parsed_frame);
    }
    return map_event(_adapter_vec_parsed_frame, _mapping_info);
}

TH2D* SJSV_eventbuilder::quick_plot_mapped_event(const mapped_event &_mapped_event, Double_t _max_adc){
    TH2D *_hist = new TH2D();
    auto _hist_name = "mapped event";
    _hist->SetTitle(_hist_name);

    auto _xaxis = _hist->GetXaxis();
    _xaxis->SetTitle("X (pixel)");
    auto _yaxis = _hist->GetYaxis();
    _yaxis->SetTitle("Y (pixel)");

    auto _x_bin_num = 105;
    auto _y_bin_num = 105;
    auto _x_bin_low = 0;
    auto _x_bin_high = 105;
    auto _y_bin_low = 0;
    auto _y_bin_high = 105;

    _hist->SetBins(_x_bin_num, _x_bin_low, _x_bin_high, _y_bin_num, _y_bin_low, _y_bin_high);

    _hist->GetXaxis()->SetRangeUser(0, 104);
    _hist->GetYaxis()->SetRangeUser(0, 104);

    if (_max_adc > 0) {
        _hist->SetMaximum(_max_adc);
    }

    for (auto i=0; i<_mapped_event.x_coords_array.size(); i++) {
        auto _x_coord = _mapped_event.x_coords_array.at(i);
        auto _y_coord = _mapped_event.y_coords_array.at(i);
        auto _cell_size = _mapped_event.cell_size_array.at(i);
        auto _value = _mapped_event.value_array.at(i);
        if (_value == 0) {
            continue;
        }
        // auto _log_value = log10(_value);
        // auto _error = _mapped_event.error_array.at(i);
        auto _x_offset = _cell_size / 2;
        auto _y_offset = _cell_size / 2;
        for (auto _x = _x_coord - _x_offset; _x < _x_coord + _x_offset; _x++) {
            for (auto _y = _y_coord - _y_offset; _y < _y_coord + _y_offset; _y++) {
                _hist->Fill(_x, _y, _value);
            }
        }
    }
    // set color palette
    gStyle->SetPalette(kSunset);
    // list of color palettes: https://root.cern.ch/doc/master/classTColor.html
    _hist->SetStats(0);
    // show color bar
    _hist->SetContour(100);
    return _hist;
}

TH2D* SJSV_eventbuilder::plot_mapped_event_calib(const mapped_event &_mapped_event, const std::vector<Double_t> &_cell_id_vec, std::vector<Double_t> &_slope_vec, const std::vector<Double_t> &_intercept_vec, Double_t _max_adc){

    LOG(INFO) << "length of cell id vec: " << _cell_id_vec.size();
    LOG(INFO) << "length of slope vec: " << _slope_vec.size();
    LOG(INFO) << "length of intercept vec: " << _intercept_vec.size();
    TH2D *_hist = new TH2D();
    auto _hist_name = "mapped event";
    _hist->SetTitle(_hist_name);

    auto _xaxis = _hist->GetXaxis();
    _xaxis->SetTitle("X (pixel)");
    auto _yaxis = _hist->GetYaxis();
    _yaxis->SetTitle("Y (pixel)");

    auto _x_bin_num = 105;
    auto _y_bin_num = 105;
    auto _x_bin_low = 0;
    auto _x_bin_high = 105;
    auto _y_bin_low = 0;
    auto _y_bin_high = 105;

    _hist->SetBins(_x_bin_num, _x_bin_low, _x_bin_high, _y_bin_num, _y_bin_low, _y_bin_high);

    _hist->GetXaxis()->SetRangeUser(0, 104);
    _hist->GetYaxis()->SetRangeUser(0, 104);

    if (_max_adc > 0) {
        _hist->SetMaximum(_max_adc);
    }

    std::vector<bool> _LG_available;
    std::vector<Double_t> _id_vec;
    std::vector<Double_t> _candidate_adc_vec;

    for (auto i=0; i<_mapped_event.x_coords_array.size(); i++) {
        auto _x_coord = _mapped_event.x_coords_array.at(i);
        auto _y_coord = _mapped_event.y_coords_array.at(i);
        auto _cell_size = _mapped_event.cell_size_array.at(i);
        auto _value = _mapped_event.value_array.at(i);
        auto _lg_value = _mapped_event.value_LG_array.at(i);
        if (_value == -1) {
            // * then it is LG
            if (_lg_value > 0) {
                // * then it is LG available
                _id_vec.push_back(_x_coord*210 + _y_coord);
                _candidate_adc_vec.push_back(_lg_value);
                _LG_available.push_back(true);
            }
            continue;
        }
    }

    // LOG(INFO) << "LG available: " << _LG_available.size();

    for (auto i=0; i<_mapped_event.x_coords_array.size(); i++) {
        auto _x_coord = _mapped_event.x_coords_array.at(i);
        auto _y_coord = _mapped_event.y_coords_array.at(i);
        auto _cell_size = _mapped_event.cell_size_array.at(i);
        auto _value = _mapped_event.value_array.at(i);
        auto _cell_id = _x_coord*210 + _y_coord;

        auto _value_to_use = _value;
        if (_value == -1) {
            // * then it is HG
            continue;
        }
        if (_value > 900) {
            //LOG(INFO) << "HG value too large: " << _value << " at cell id " << _cell_id;
            // print the cell id
            // for (auto _id : _id_vec) {
            //     LOG(INFO) << "Cell id: " << _id;
            // }
            // * search for LG
            auto _index = std::find(_id_vec.begin(), _id_vec.end(), _cell_id) - _id_vec.begin();
            if (_index == _id_vec.size()) {
                // * then no LG available
                continue;
            } else {
                auto _lg_value = _candidate_adc_vec.at(_index);
                LOG(INFO) << "Found LG candidate: " << _candidate_adc_vec.at(_index);
                // * then LG available
                if (_LG_available.at(_index)) {
                    // * then LG is available
                    auto _slope_index = std::find(_cell_id_vec.begin(), _cell_id_vec.end(), _cell_id) - _cell_id_vec.begin();
                    if (_slope_index == _cell_id_vec.size()) {
                        // * then no slope available
                        continue;
                    }
                    auto _slope = _slope_vec.at(_slope_index);
                    auto _intercept = _intercept_vec.at(_slope_index);
                    LOG(INFO) << "Substituting HG with LG: " << _value << " -> " << _lg_value << " with slope " << _slope << " and intercept " << _intercept << " at cell id " << _cell_id;
                    _value_to_use = (_lg_value - _intercept) / _slope;
                } else {
                    // * then LG is not available
                    continue;
                }
            }
        }
        // auto _log_value = log10(_value);
        // auto _error = _mapped_event.error_array.at(i);
        auto _x_offset = _cell_size / 2;
        auto _y_offset = _cell_size / 2;
        for (auto _x = _x_coord - _x_offset; _x < _x_coord + _x_offset; _x++) {
            for (auto _y = _y_coord - _y_offset; _y < _y_coord + _y_offset; _y++) {
                // LOG(INFO) << "Filling cell " << _x << ", " << _y << " with adc " << _value_to_use;
                _hist->Fill(_x, _y, _value_to_use);
            }
        }
    }
    // set color palette
    gStyle->SetPalette(kSunset);
    // list of color palettes: https://root.cern.ch/doc/master/classTColor.html
    _hist->SetStats(0);
    //
    _hist->SetContour(100);
    return _hist;
}

TH2D* SJSV_eventbuilder::quick_plot_multiple_channels_hist(std::vector<uint16_t> _vec_channel, Int_t _bin_num, Double_t _bin_low, Double_t _bin_high){
    TH2D* _hist = new TH2D();
    auto _hist_name = "multiple channels";
    _hist->SetTitle(_hist_name);

    _hist->GetXaxis()->SetTitle("Channel");
    _hist->GetYaxis()->SetTitle("ADC");
    auto _x_bin_num = _vec_channel.size();
    auto _x_bin_low = *std::min_element(_vec_channel.begin(), _vec_channel.end());
    auto _x_bin_high = _vec_channel.size() + _x_bin_low;
    auto _y_bin_num = _bin_num;
    auto _y_bin_low = _bin_low;
    auto _y_bin_high = _bin_high;

    _hist->SetBins(_x_bin_num, _x_bin_low, _x_bin_high, _y_bin_num, _y_bin_low, _y_bin_high);

    auto _frame_num = vec_parsed_frame_ptr->size();
    for (auto i=0; i<_frame_num; i++) {
        auto _parsed_frame = vec_parsed_frame_ptr->at(i);
        auto _channel = _parsed_frame.uni_channel;
        auto _adc = _parsed_frame.adc;
        if (std::find(_vec_channel.begin(), _vec_channel.end(), _channel) != _vec_channel.end()) {
            _hist->Fill(_channel, _adc);
            // LOG(DEBUG) << "Filling channel " << _channel << " with adc " << _adc;
        }
    }
    _hist->SetStats(0);
    gStyle->SetPalette(kBird);
    return _hist;
}

bool SJSV_eventbuilder::reconstruct_event(Double_t _threshold_time_ns){
    auto _parsed_frame_num = vec_parsed_frame_ptr->size();
    if (_parsed_frame_num == 0) {
        LOG(ERROR) << "Parsed frame vector is empty";
        return false;
    }
    if (_threshold_time_ns < 0) {
        LOG(ERROR) << "Threshold time is negative";
        return false;
    }
    if (vec_parsed_event_ptr->size() != 0) {
        LOG(WARNING) << "Parsed event is not empty, deleting old data";
        vec_parsed_event_ptr->clear();
    }

    auto _last_frame_time = vec_parsed_frame_ptr->at(0).time_ns;
    uint32_t _current_event_id = 1;
    std::vector<parsed_frame*> _candidate_frames;
    for (auto _frame_index=0; _frame_index<_parsed_frame_num; _frame_index++){
        auto _parsed_frame = vec_parsed_frame_ptr->at(_frame_index);
        auto _time_ns = _parsed_frame.time_ns;
        auto _uni_channel = _parsed_frame.uni_channel;
        
        if (abs(_time_ns - _last_frame_time) < _threshold_time_ns){
        } else {
            if (_candidate_frames.size() > 0) {
                // check repeated channel
                std::vector<uint16_t> _vec_channel;
                for (auto _frame_ptr : _candidate_frames) {
                    _vec_channel.push_back(_frame_ptr->uni_channel);
                    _frame_ptr->event_id = _current_event_id;
                }
                std::sort(_vec_channel.begin(), _vec_channel.end());
                auto _it = std::unique(_vec_channel.begin(), _vec_channel.end());
                _vec_channel.erase(_it, _vec_channel.end());

                if (_vec_channel.size() != _candidate_frames.size()) {
                    LOG(WARNING) << "Repeated channel found, skipping this event";
                    _candidate_frames.clear();
                    continue;
                }

                auto _candidate_event_adc_sum = 0;
                for (auto _frame_ptr : _candidate_frames) {
                    _candidate_event_adc_sum += _frame_ptr->adc;
                }
                if (_candidate_event_adc_sum < 500) {
                    LOG(WARNING) << "Event ADC sum too small, skipping this event";
                    _candidate_frames.clear();
                    continue;
                }

                auto _candidate_event_chn_num = _candidate_frames.size();
                if (_candidate_event_chn_num < 20) {
                    LOG(WARNING) << "Event channel number too small, skipping this event";
                    _candidate_frames.clear();
                    continue;
                }

                auto _parsed_event = new parsed_event();
                _parsed_event->frames_ptr = _candidate_frames;
                _parsed_event->id = _current_event_id;
                _current_event_id++;

                vec_parsed_event_ptr->push_back(*_parsed_event);
            }
            _candidate_frames.clear();
        }
        _candidate_frames.push_back(&vec_parsed_frame_ptr->at(_frame_index));
        _last_frame_time = _time_ns;
    }

    return true;
}

bool SJSV_eventbuilder::reconstruct_event_list(Double_t _threshold_time_ns){
    int _seed_list_max_len = RECONSTRUCTION_LIST_LEN;
    auto _parsed_frame_num = vec_parsed_frame_ptr->size();
    if (_parsed_frame_num == 0) {
        LOG(ERROR) << "Parsed frame vector is empty";
        return false;
    }
    if (_threshold_time_ns < 0) {
        LOG(ERROR) << "Threshold time is negative";
        return false;
    }
    if (vec_parsed_event_ptr->size() != 0) {
        LOG(WARNING) << "Parsed event is not empty, deleting old data";
        vec_parsed_event_ptr->clear();
    }

    auto _too_small_event_cnt = 0;
    uint32_t _current_event_id = 1;
    std::vector<bool> _is_frame_used(_parsed_frame_num, false);
    for (auto _frame_index=0; _frame_index<_parsed_frame_num; _frame_index++){
        if (_is_frame_used.at(_frame_index)) {
            continue;
        }
        // looking for hits within the time window
        std::vector<parsed_frame*> _candidate_frames;
        int _smaller_limit = _frame_index + RECONSTRUCTION_CHK_LEN;
        if (_smaller_limit > _parsed_frame_num) {
            _smaller_limit = _parsed_frame_num;
        }
        for (auto _search_index=_frame_index; _search_index<_smaller_limit; _search_index++){
            if (_is_frame_used.at(_search_index)) {
                continue;
            }
            auto _parsed_frame = vec_parsed_frame_ptr->at(_search_index);
            auto _time_ns = _parsed_frame.time_ns;
            if (abs(_time_ns - vec_parsed_frame_ptr->at(_frame_index).time_ns) < _threshold_time_ns){
                _candidate_frames.push_back(&vec_parsed_frame_ptr->at(_search_index));
                _is_frame_used.at(_search_index) = true;
            }
        }
        // check if the candidate frames are legal
        if (_candidate_frames.size() < MINIMUM_EVENT_HIT) {
            _too_small_event_cnt++;
            continue;
        }

        // save the event
        auto _parsed_event = new parsed_event();
        _parsed_event->frames_ptr = _candidate_frames;
        _parsed_event->id = _current_event_id;
        _current_event_id++;

        vec_parsed_event_ptr->push_back(*_parsed_event);
        // if (vec_parsed_event_ptr->size() == 1) {
        //     LOG(DEBUG) << "Event " << _parsed_event->id << " reconstructed with " << _candidate_frames.size() << " frames";
        //     for (auto _frame_ptr : vec_parsed_event_ptr->at(0).frames_ptr) {
        //         LOG(DEBUG) << "Frame " << _frame_ptr->uni_channel << " " << _frame_ptr->adc << " " << _frame_ptr->time_ns;
        //     }
        // }

        // print info
    }

    return true;
}

TH1D* SJSV_eventbuilder::quick_plot_event_chnnum_hist(int max_channel_num){
    TH1D* _hist = new TH1D();
    auto _hist_name = "event channel count";
    _hist->SetTitle(_hist_name);
    _hist->SetBins(max_channel_num, 0, max_channel_num);

    for (auto _event: *vec_parsed_event_ptr) {
        _hist->Fill(_event.frames_ptr.size());
    }
    
    _hist->SetStats(true);
    _hist->GetXaxis()->SetTitle("Channel number");
    _hist->GetYaxis()->SetTitle("Event number");
    return _hist;
}

TH1D* SJSV_eventbuilder::quick_plot_event_adc_hist(Int_t _bin_num, Double_t _bin_low, Double_t _bin_high){
    TH1D* _hist = new TH1D();
    auto _hist_name = "event adc";
    _hist->SetTitle(_hist_name);

    _hist->SetBins(_bin_num, _bin_low, _bin_high);

    for (auto _event: *vec_parsed_event_ptr) {
        Double_t _adc_sum = 0;
        for (auto _frame_ptr : _event.frames_ptr) {
            if (is_frame_HG(*_frame_ptr))
                _adc_sum += _frame_ptr->adc;
        }
        _hist->Fill(_adc_sum);
    }

    _hist->SetStats(true);
    _hist->GetXaxis()->SetTitle("ADC");
    _hist->GetYaxis()->SetTitle("Event number");
    return _hist;
}

TH1D* SJSV_eventbuilder::quick_plot_event_LG_adc_hist(Int_t _bin_num, Double_t _bin_low, Double_t _bin_high){
    TH1D* _hist = new TH1D();
    auto _hist_name = "event LG adc";
    _hist->SetTitle(_hist_name);

    _hist->SetBins(_bin_num, _bin_low, _bin_high);

    for (auto _event: *vec_parsed_event_ptr) {
        Double_t _adc_sum = 0;
        for (auto _frame_ptr : _event.frames_ptr) {
            if (!is_frame_HG(*_frame_ptr))
                _adc_sum += _frame_ptr->adc;
        }
        _hist->Fill(_adc_sum);
    }

    _hist->SetStats(true);
    _hist->GetXaxis()->SetTitle("ADC");
    _hist->GetYaxis()->SetTitle("Event number");
    return _hist;
}

TH2D* SJSV_eventbuilder::quick_plot_mapped_events_sum(void){
    std::vector<uint16_t> _vec_channel;
    for (auto _parsed_frame : *vec_parsed_frame_ptr) {
        _vec_channel.push_back(_parsed_frame.uni_channel);
    }
    std::sort(_vec_channel.begin(), _vec_channel.end());
    auto _it = std::unique(_vec_channel.begin(), _vec_channel.end());
    _vec_channel.erase(_it, _vec_channel.end());

    // create a summed event
    parsed_event _summed_event;
    for (auto _chn: _vec_channel){
        parsed_frame* _parsed_frame = new parsed_frame();
        _parsed_frame->uni_channel = _chn;
        _parsed_frame->adc = 0;
        _parsed_frame->time_ns = 0;
        _parsed_frame->event_id = 0;
        _summed_event.frames_ptr.push_back(_parsed_frame);
    }

    for (auto _parsed_frame : *vec_parsed_frame_ptr) {
        auto _uni_channel = _parsed_frame.uni_channel;
        if (_uni_channel > 20000)
            LOG(ERROR) << "Channel number too large in plot: " << _uni_channel;
        auto _adc = _parsed_frame.adc;
        for (auto _frame_ptr : _summed_event.frames_ptr) {
            if (_frame_ptr->uni_channel == _uni_channel) {
                _frame_ptr->adc += _adc;
            }
        }
    }

    auto max_adc_sum = 0;
    for (auto _frame_ptr : _summed_event.frames_ptr) {
        if (_frame_ptr->adc > max_adc_sum) {
            max_adc_sum = _frame_ptr->adc;
        }
    }

    LOG(INFO) << "Max ADC sum: " << max_adc_sum;

    auto _mapped_event = map_event(_summed_event, *mapping_info_ptr);

    for (auto _parsed_frame : *vec_parsed_frame_ptr) {
        auto _uni_channel = _parsed_frame.uni_channel;
        if (_uni_channel > 20000)
            LOG(ERROR) << "Channel number too large in plot: " << _uni_channel;
    }

    return quick_plot_mapped_event(_mapped_event, max_adc_sum);
}

TH2D* SJSV_eventbuilder::quick_plot_mapped_events_sum2(void){
    // create a summed event
    parsed_event _summed_event;

    for (auto i=0; i<500; i++){
        auto _event = vec_parsed_event_ptr->at(i);
        for (auto _frame_ptr: _event.frames_ptr){
            // see if the summed event has this channel
            bool _has_channel = false;
            int _frame_index = 0;
            for (auto _frame_cnt=0; _frame_cnt<_summed_event.frames_ptr.size(); _frame_cnt++){
                if (_summed_event.frames_ptr.at(_frame_cnt)->uni_channel == _frame_ptr->uni_channel){
                    _has_channel = true;
                    _frame_index = _frame_cnt;
                    break;
                }
            }
            if (!_has_channel){
                parsed_frame* _parsed_frame = new parsed_frame();
                _parsed_frame->uni_channel = _frame_ptr->uni_channel;
                _parsed_frame->adc = _frame_ptr->adc;
                _parsed_frame->time_ns = _frame_ptr->time_ns;
                _parsed_frame->event_id = _frame_ptr->event_id;
                _summed_event.frames_ptr.push_back(_parsed_frame);
            } else {
                _summed_event.frames_ptr.at(_frame_index)->adc += _frame_ptr->adc;
            }
        }
    }

    // Fill the rest channels with 1
    for (auto i=0; i < 16*64; i++){
        bool _has_channel = false;
        for (auto _frame_ptr: _summed_event.frames_ptr){
            if (_frame_ptr->uni_channel == i){
                _has_channel = true;
                break;
            }
        }
        if (!_has_channel){
            parsed_frame* _parsed_frame = new parsed_frame();
            _parsed_frame->uni_channel = i;
            _parsed_frame->adc = 1;
            _parsed_frame->time_ns = 0;
            _parsed_frame->event_id = 0;
            _summed_event.frames_ptr.push_back(_parsed_frame);
        }
    }

    auto max_adc_sum = 0;
    for (auto _frame_ptr : _summed_event.frames_ptr) {
        if (_frame_ptr->adc > max_adc_sum) {
            max_adc_sum = _frame_ptr->adc;
        }
    }

    LOG(INFO) << "Max ADC sum: " << max_adc_sum;

    auto _mapped_event = map_event(_summed_event, *mapping_info_ptr);

    for (auto _parsed_frame : *vec_parsed_frame_ptr) {
        auto _uni_channel = _parsed_frame.uni_channel;
        if (_uni_channel > 20000)
            LOG(ERROR) << "Channel number too large in plot: " << _uni_channel;
    }

    return quick_plot_mapped_event(_mapped_event, max_adc_sum);
}