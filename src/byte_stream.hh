#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm> // added for using min function

using namespace std;

class Reader;
class Writer;

class ByteStream
{
public:
 explicit ByteStream(uint64_t capacity);

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; };       // Signal that the stream suffered an error.
  bool has_error() const { return error_; }; // Has the stream had an error?

protected:
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  string buffer; 
  uint64_t capacity_;
  bool error_ {};
  bool writer_closed = false; // used to keep track of well whether the writer end has been closed
  uint64_t bytes_pushed_count = 0; // push count
  uint64_t bytes_popped_count = 0;  // pop count
  uint64_t bytes_to_push = 0; // total bytes to push
  uint64_t bytes_to_pop = 0; // total bytes to pop
  
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // Push data to stream, but only as much as available capacity allows.
  void close();                  // Signal that the stream has reached its ending. Nothing more will be written.

  bool is_closed() const;              // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // Peek at the next bytes in the buffer
  void pop( uint64_t len );      // Remove `len` bytes from the buffer

  bool is_finished() const;        // Is the stream finished (closed and fully popped)?
  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};

/*
 * read: A (provided) helper function thats peeks and pops up to `max_len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t max_len, std::string& out );
