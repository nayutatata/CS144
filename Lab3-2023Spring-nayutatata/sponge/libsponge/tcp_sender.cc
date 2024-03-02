#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}
uint64_t TCPSender::bytes_in_flight() const {
    return flyingbytes;
}

void TCPSender::fill_window() { 
#ifdef DEBUG
    cout << "Now fill_window" << endl;
#endif
    size_t wnd = rwnd;
    if (rwnd==0)
        wnd = 1;
    //size_t remain = wnd - flyingbytes;
    while (wnd>flyingbytes) {
        TCPSegment seg{};
        TCPHeader &segh = seg.header();
        if (havesyn == false) {
            havesyn = true;
            segh.syn = 1;
        }
        segh.seqno = next_seqno();
        string data = _stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, wnd - flyingbytes - segh.syn));
        if (havefin==false&&_stream.eof()&&wnd>data.size()+flyingbytes){
            havefin = true;
            segh.fin = 1;
        }
        seg.payload() = Buffer(move(data));

        if (seg.length_in_sequence_space()==0)
            return;
        else {
            if (flyingqueue.empty()){
                timeout = _initial_retransmission_timeout;
                timer = 0;
            }
            _segments_out.push(seg);
            flyingqueue.push({_next_seqno, seg});
            _next_seqno += seg.length_in_sequence_space();
            flyingbytes += seg.length_in_sequence_space();
            if (segh.fin)
                return;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
#ifdef DEBUG
    cout<<"Here ack_received"<<endl;
    abort();
#endif
    uint64_t absindex = unwrap(ackno, _isn, _next_seqno);
    if (absindex<=_next_seqno){
       while (!flyingqueue.empty()){
            std::pair<uint64_t, TCPSegment> temp = flyingqueue.front();
            if (temp.first+temp.second.length_in_sequence_space()>absindex)
                break;
            flyingbytes -= temp.second.length_in_sequence_space();
            flyingqueue.pop();
            timeout = _initial_retransmission_timeout;
            timer = 0;
        }
        acc_count = 0;
        rwnd = window_size;
        fill_window();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    timer += ms_since_last_tick;
    if (!flyingqueue.empty()&&timer>=timeout){
        if (rwnd>0)
            timeout *= 2;
        timer = 0;
        _segments_out.push(flyingqueue.front().second);
        acc_count++;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return acc_count; }

void TCPSender::send_empty_segment() { 
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
