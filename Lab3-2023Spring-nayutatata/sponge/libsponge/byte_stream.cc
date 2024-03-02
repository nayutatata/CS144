#include "byte_stream.hh"

#include <iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// You will need to add private members to the class declaration in `byte_stream.hh`

//
template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacit) : buffer(), capacity(capacit), haswritten(0), hasread(0), is_end(false) {}

size_t ByteStream::write(const string &data) {
    if (is_end)
        return 0;
    size_t will_write = data.size();
    size_t remain = remaining_capacity();
    if (remain < data.size())
        will_write = remain;
    haswritten += will_write;
    for (unsigned i = 0; i < will_write; i++)
        buffer.push_back(data[i]);
    return will_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t reallen = len;
    if (reallen > buffer.size()) {
        reallen = buffer.size();
    }
    string ans(buffer.begin(), buffer.begin() + reallen);
    return ans;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t reallen = len;
    if (reallen > buffer.size()) {
        reallen = buffer.size();
    }
    hasread += reallen;
    for (unsigned i = 0; i < reallen; i++) {
        buffer.pop_front();
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string temp = peek_output(len);
    pop_output(len);
    return temp;
}

void ByteStream::end_input() { is_end = true; }

bool ByteStream::input_ended() const { return is_end; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.empty(); }

bool ByteStream::eof() const { return is_end && buffer.empty(); }

size_t ByteStream::bytes_written() const { return haswritten; }

size_t ByteStream::bytes_read() const { return hasread; }

size_t ByteStream::remaining_capacity() const { return capacity - buffer.size(); }