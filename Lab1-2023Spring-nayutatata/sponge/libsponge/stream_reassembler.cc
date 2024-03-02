#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : record(capacity),valid(capacity,false),first_unread(0),first_unassembled(0),num_unassembled(0),end_id(-1),_output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof){
        end_id = index + data.size();
    }
    //首先更新变量
    first_unread = _output.bytes_read();
    first_unassembled = _output.bytes_written();
    uint64_t first_unaccepted = first_unread + _capacity;
    if (index>=first_unaccepted||index+data.size()<=first_unassembled){
        if (end_id == _output.bytes_written()) {
            // while(1)
            //   ;
            _output.end_input();
        }
    }
    uint64_t be = max(first_unassembled, index);
    uint64_t en = min(first_unaccepted, index + data.size());
    for (uint64_t i = be; i < en;i++){
        if(!valid[i-first_unassembled]){
            record[i - first_unassembled] = data[i - index];
            valid[i - first_unassembled] = true;
            num_unassembled++;
        }
    }
    while(valid.front()){
        string a = "";
        a.push_back(record.front());
        _output.write(a);
        num_unassembled--;
        record.pop_front();
        record.push_back('w');
        valid.pop_front();
        valid.push_back(false);
    }
    if (end_id==_output.bytes_written()){
        //while(1)
          //  ;
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return num_unassembled; }

bool StreamReassembler::empty() const { return num_unassembled == 0; }