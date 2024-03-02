#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader h = seg.header();
    if (syn == 0) {
        if (h.syn) {
            isn = h.seqno;
            syn = 1;
        } else
            return;
    }
    uint64_t checkpoint = _reassembler.stream_out().bytes_written() + 1;
    uint64_t absno = unwrap(h.syn ? h.seqno + 1 : h.seqno, isn, checkpoint);
    uint64_t stream_index = absno - 1;
    _reassembler.push_substring(seg.payload().copy(), stream_index, h.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (syn == 0) {
        return {};
    }
    uint64_t ackno = _reassembler.stream_out().bytes_written() + 1;
    if (_reassembler.stream_out().input_ended()) {
        return WrappingInt32(isn) + ackno + 1;
    }
    return WrappingInt32(isn) + ackno;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
