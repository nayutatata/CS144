#include "tcp_connection.hh"

#include <iostream>
// Dummy implementation of a TCP connection

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check`.
template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;
#define SEND                                                                                                           \
    do {                                                                                                               \
        while (!_sender.segments_out().empty()) {                                                                      \
            TCPSegment s = _sender.segments_out().front();                                                             \
            TCPHeader &head = s.header();                                                                              \
            _sender.segments_out().pop();\
            if (_receiver.ackno().has_value()) {\
                head.ack = true;\
                head.ackno = _receiver.ackno().value();\
                head.win = _receiver.window_size();\
            }\
            _segments_out.push(s);                                                                                   \
        }} while (0)
#define SET_ALL_ERROR                                                                                                  \
    do {                                                                                                               \
        _receiver.stream_out().set_error();                                                                            \
        _sender.stream_in().set_error();                                                                               \
        _linger_after_streams_finish = false;                                                                          \
        alive = false;                                                                                            \
    } while (0)
int TCPConnection::check_state(std::string r,std::string s){
    if (r==TCPReceiverStateSummary::SYN_RECV&&s==TCPSenderStateSummary::CLOSED){
        return 0;
    } 
    else if (r == TCPReceiverStateSummary::FIN_RECV && s == TCPSenderStateSummary::SYN_ACKED) {
        return 1;
    } 
    else if (r == TCPReceiverStateSummary::FIN_RECV && s == TCPSenderStateSummary::FIN_ACKED) {
        return 2;
    }
    return -1;
}
size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return waittime; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    bool is_data = seg.length_in_sequence_space();
    waittime = 0;
    _receiver.segment_received(seg);
    TCPHeader h = seg.header();
    if (h.rst){
        /*_receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        alive = true;
        _linger_after_streams_finish = false;*/
        SET_ALL_ERROR;
        return;
    }
    if (h.ack){
        _sender.ack_received(h.ackno, h.win);
        if (is_data&&!_sender.segments_out().empty())
            is_data = false;
    }
    int code=check_state(TCPState::state_summary(_receiver),TCPState::state_summary(_sender));
    if (code==0){
        connect();
        return;
    }
    if (code==1){
        _linger_after_streams_finish=false;
    }
    if (code==2&&!_linger_after_streams_finish){
        alive = false;
        return;
    }
    if (is_data)
        _sender.send_empty_segment();
    SEND;
}

bool TCPConnection::active() const { return alive; }

size_t TCPConnection::write(const string &data) { 
    size_t ans = _sender.stream_in().write(data);
    _sender.fill_window();
    SEND;
    return ans;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
     if (_sender.consecutive_retransmissions()>_cfg.MAX_RETX_ATTEMPTS){
        //tick可能导致sender的队列新增了一个重传包，但是此时我们需要把所有的数据视为无效，所以需要将此时的包都去掉
        //while(!_sender.segments_out().empty()){
            _sender.segments_out().pop();
        //}
            TCPSegment seg = _sender.send_empty_segment();
            seg.header().rst = true;
            _segments_out.push(seg);
            SET_ALL_ERROR;
            return;
     }
     SEND;
     waittime += ms_since_last_tick;
     if (check_state(TCPState::state_summary(_receiver),TCPState::state_summary(_sender))==2&&_linger_after_streams_finish&&waittime>=10*_cfg.rt_timeout){
        alive = false;
        _linger_after_streams_finish = false;
     }
}

void TCPConnection::end_input_stream() { 
    _sender.stream_in().end_input(); 
    //这里需要发送FIN报文段，事实上，在fill_window中已经实现了相应的功能
    //在上层我们只需要让sender再发一个包就行了
    _sender.fill_window();
    SEND;
}

void TCPConnection::connect() { 
    _sender.fill_window();
    alive = true;
    //在这之前只是将带有SYN的包push到了sender的队列中，但是没有真正的发送
    SEND;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            SET_ALL_ERROR;
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
