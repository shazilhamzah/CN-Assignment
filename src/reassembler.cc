#include "reassembler.hh"
#include "byte_stream.hh"
#include "debug.hh"
#include <algorithm>
#include <iostream>
#include <map>

using namespace std;


void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( is_last_substring ) {
    _eof = true;
    _eof_index = first_index + data.length();
  }

  if ( output_.writer().is_closed() )
    return;

  // handles if the last substring is empty
  if ( data.empty() && is_last_substring && first_index == next_expected_index ) {
    output_.writer().close();
    return;
  }

  // exit if string is already written
  uint64_t end_index = first_index + data.length();
  if ( end_index <= next_expected_index )
    return;

  // skip if the string is out of the writer buffer capacity
  uint64_t capacity_limit = next_expected_index + output_.writer().available_capacity();
  if ( first_index >= capacity_limit )
    return;

  // trims the data to fit within the reassembler window bounds 
  if ( first_index < next_expected_index ) {
    data = data.substr( next_expected_index - first_index );
    first_index = next_expected_index;
  }
  if ( first_index + data.length() > capacity_limit ) {
    data = data.substr( 0, capacity_limit - first_index );
  }

  // in case, after trimming, the data is empty
  if ( data.empty() )
    return;

  // handle overlaps by removing conflicting segments
  auto it = unpushed_data.lower_bound( first_index );
  if ( it != unpushed_data.begin() ) {
    auto prev = std::prev( it );
    if ( prev->first + prev->second.length() > first_index ) {
      uint64_t overlap = prev->first + prev->second.length() - first_index;
      if ( overlap >= data.length() )
        return;
      data = data.substr( overlap );
      first_index += overlap;
    }
  }

  // remove overlapping segments
  end_index = first_index + data.length();
  while ( it != unpushed_data.end() && it->first < end_index ) {
    if ( it->first + it->second.length() <= end_index ) {
      it = unpushed_data.erase( it );
    } else {
      data = data.substr( 0, it->first - first_index );
      break;
    }
  }

  // store segment if not empty
  if ( !data.empty() ) {
    unpushed_data[first_index] = data;
  }

  // write contiguous data
  while ( true ) {
    auto next_it = unpushed_data.find( next_expected_index );
    if ( next_it == unpushed_data.end() )
      break;

    const string& segment = next_it->second;
    uint64_t available = output_.writer().available_capacity();

    if ( segment.length() <= available ) {
      output_.writer().push( segment );
      next_expected_index += segment.length();
      unpushed_data.erase( next_it );
    } else if ( available > 0 ) {
      output_.writer().push( segment.substr( 0, available ) );
      next_expected_index += available;
      unpushed_data[next_expected_index] = segment.substr( available );
      unpushed_data.erase( next_it );
      break;
    } else {
      break;
    }
  }

  // close if EOF reached
  if ( _eof && next_expected_index >= _eof_index ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total = 0;

  for ( const auto& entry : this->unpushed_data )
    total += entry.second.size();
  // debug( "unimplemented count_bytes_pending() called" );
  return total;
}
