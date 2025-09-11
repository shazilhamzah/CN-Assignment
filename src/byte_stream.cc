#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity): buffer(), capacity_(capacity) {} // constructor intialization

void Writer::push( string data )
{
  bytes_to_push = min(data.length(), available_capacity()); // using min to ensure that data pushed does not exceed avail capacity
  buffer.append(data, 0, bytes_to_push); // append the string to the buffer
  bytes_pushed_count = bytes_pushed_count + bytes_to_push; // update the total push count
}

void Writer::close()
{
  writer_closed = true; // set the value for writer close to true
}

bool Writer::is_closed() const
{
  return writer_closed; // return the value of the variable to see if the writer is closed or not
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer.length(); // return the avaialble amount by subtracting the total capacity with current buffer size
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_count; // return the number of bytes that have been pushed
}

string_view Reader::peek() const
{
  return buffer; // the string view library enables us to see the contents without having to retrieve the contents as a temp window
}

void Reader::pop( uint64_t len )
{
  bytes_to_pop = min(len, bytes_buffered()); // uses similar initial logic as push, it ensures that kay sirf jitni bytes hein wo delete houn minus mein na jaye
  buffer.erase(0, bytes_to_pop); // 0 is the starting position and it goes upto the number of bytes to pop
  bytes_popped_count += bytes_to_pop; // update the count for bytes popped
}

bool Reader::is_finished() const
{
  return (writer_closed && buffer.empty()); // check if writer closed is true and the buffer is empty (nothing to read basically)
}

uint64_t Reader::bytes_buffered() const
{
  return buffer.length(); // just check the length
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_count; // return the popped count 
}
