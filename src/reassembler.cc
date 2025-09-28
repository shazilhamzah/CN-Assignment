#include "reassembler.hh"
#include "byte_stream.hh"
#include "debug.hh"
#include <algorithm>
#include <iostream>
#include <map>

using namespace std;

/*
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{

  if ( is_last_substring ) {
    this->_eof = true;
    this->_eof_index = first_index + data.size();
  }

  // skip already being pushed data
  if ( first_index + data.size() <= next_expected_index ) {
    if ( _eof && this->unpushed_data.empty() && this->next_expected_index == this->_eof_index )
      output_.writer().close();
    return;
  }
  if ( first_index < this->next_expected_index ) {
    uint64_t skip = next_expected_index - first_index;
    data = data.substr( skip );
    first_index = next_expected_index;
  }

  // calculating window capacity
  size_t available = output_.writer().available_capacity();
  uint64_t window_end = next_expected_index + available;

  if ( first_index >= window_end ) {
    if ( _eof && this->unpushed_data.empty() && this->next_expected_index == this->_eof_index )
      output_.writer().close();
    return;
  }

  // trim data if partial data will be included in window
  if ( first_index + data.size() > window_end ) {
    data = data.substr( 0, window_end - first_index );
  }

  if ( this->next_expected_index != first_index ) {
    size_t pending_bytes = count_bytes_pending();
    size_t available_capacity = output_.writer().available_capacity();
    size_t free_capacity = ( available_capacity > pending_bytes ) ? ( available_capacity - pending_bytes ) : 0;

    if ( data.size() > free_capacity ) {
      if ( _eof && this->unpushed_data.empty() && this->next_expected_index == this->_eof_index )
        output_.writer().close();
      return;
    }

    this->unpushed_data[first_index] = move( data );
    cout << count_bytes_pending();

    if ( _eof && this->unpushed_data.empty() && this->next_expected_index == this->_eof_index )
      output_.writer().close();
    return;
  }

  // check for available capacity here
  size_t remaining = output_.writer().available_capacity();
  size_t push_len = min( remaining, data.size() );

  // push as much data as you can
  output_.writer().push( data.substr( 0, push_len ) );
  this->next_expected_index += push_len;

  if ( push_len < data.size() )
    this->unpushed_data[this->next_expected_index] = data.substr( push_len );

  // checking unpushed data if those bad boys deserve to be pushed
  for ( auto it = this->unpushed_data.begin(); it != this->unpushed_data.end(); ) {
    if ( it->first == next_expected_index ) {
      remaining = output_.writer().available_capacity();
      push_len = min( remaining, it->second.size() );
      output_.writer().push( it->second.substr( 0, push_len ) );
      this->next_expected_index += push_len;

      if ( push_len == it->second.size() )
        it = this->unpushed_data.erase( it );
      else {
        it->second.erase( 0, push_len );
        break;
      }
    } else
      it++;
  }

  // checking if we should close the write buffer in case it is eof and the unpushed_data map is empty
  if ( _eof && this->unpushed_data.empty() && this->next_expected_index == this->_eof_index ) {
    output_.writer().close();
  }

  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
}

*/

// void Reassembler::insert( uint64_t first_index, std::string data, bool is_last_substring )
// {
//   // Handle EOF marker
//   if ( is_last_substring ) {
//     _eof = true;
//     _eof_index = first_index + data.length();
//   }

//   // If stream is already closed, ignore
//   if ( output_.writer().is_closed() ) {
//     return;
//   }

//   // Handle empty string case - if it's the last substring and empty, close immediately
//   if ( data.empty() && is_last_substring && first_index == next_expected_index ) {
//     output_.writer().close();
//     return;
//   }

//   // Calculate available capacity for unassembled data
//   uint64_t available_capacity = output_.writer().available_capacity();
//   uint64_t bytes_buffered = 0;
//   for ( const auto& segment : unpushed_data ) {
//     bytes_buffered += segment.second.length();
//   }

//   // Discard data that's already been written
//   if ( first_index + data.length() <= next_expected_index ) {
//     return;
//   }

//   // Trim data that's already been written (handle overlap at beginning)
//   if ( first_index < next_expected_index ) {
//     uint64_t trim_amount = next_expected_index - first_index;
//     data = data.substr( trim_amount );
//     first_index = next_expected_index;
//   }

//   // Discard data beyond capacity
//   uint64_t max_acceptable_index = next_expected_index + available_capacity;
//   if ( first_index >= max_acceptable_index ) {
//     return; // Entire segment is beyond capacity
//   }

//   // Trim data that exceeds capacity
//   if ( first_index + data.length() > max_acceptable_index ) {
//     data = data.substr( 0, max_acceptable_index - first_index );
//   }

//   // If no data remains after trimming, return
//   if ( data.empty() ) {
//     return;
//   }

//   // Handle overlaps with existing segments in unpushed_data
//   auto it = unpushed_data.lower_bound( first_index );

//   // Check overlap with previous segment
//   if ( it != unpushed_data.begin() ) {
//     auto prev_it = std::prev( it );
//     uint64_t prev_end = prev_it->first + prev_it->second.length();

//     if ( prev_end > first_index ) {
//       // Overlap with previous segment
//       if ( prev_end >= first_index + data.length() ) {
//         // New segment is completely contained in previous segment
//         return;
//       } else {
//         // Partial overlap - trim the beginning of new data
//         uint64_t overlap = prev_end - first_index;
//         data = data.substr( overlap );
//         first_index += overlap;
//       }
//     }
//   }

//   // Handle overlaps with subsequent segments
//   while ( it != unpushed_data.end() && it->first < first_index + data.length() ) {
//     uint64_t segment_start = it->first;
//     uint64_t segment_end = segment_start + it->second.length();
//     uint64_t new_data_end = first_index + data.length();

//     if ( segment_start <= first_index && segment_end >= new_data_end ) {
//       // New segment is completely contained in existing segment
//       return;
//     } else if ( segment_start >= first_index && segment_end <= new_data_end ) {
//       // Existing segment is completely contained in new segment
//       it = unpushed_data.erase( it );
//     } else if ( segment_start < new_data_end ) {
//       // Partial overlap
//       if ( segment_start > first_index ) {
//         // Trim end of new data
//         data = data.substr( 0, segment_start - first_index );
//         break;
//       } else {
//         // This shouldn't happen given our previous checks, but handle anyway
//         it = unpushed_data.erase( it );
//       }
//     } else {
//       break;
//     }
//   }

//   // Insert the new segment if it has data
//   if ( !data.empty() ) {
//     unpushed_data[first_index] = data;
//   }

//   // Try to write contiguous data starting from next_expected_index
//   auto write_it = unpushed_data.find( next_expected_index );
//   while ( write_it != unpushed_data.end() && write_it->first == next_expected_index ) {
//     const std::string& segment_data = write_it->second;

//     // Check available capacity before writing
//     uint64_t available_space = output_.writer().available_capacity();

//     if ( available_space == 0 ) {
//       // No space available, stop writing
//       break;
//     }

//     if ( segment_data.length() <= available_space ) {
//       // Can write entire segment
//       output_.writer().push( segment_data );
//       next_expected_index += segment_data.length();
//       write_it = unpushed_data.erase( write_it );

//       // Check if next segment is contiguous
//       if ( write_it != unpushed_data.end() && write_it->first == next_expected_index ) {
//         continue;
//       } else {
//         break;
//       }
//     } else {
//       // Can only write part of the segment
//       std::string writable_part = segment_data.substr( 0, available_space );
//       std::string remaining_part = segment_data.substr( available_space );

//       output_.writer().push( writable_part );
//       next_expected_index += available_space;

//       // Update the segment with remaining data
//       unpushed_data[next_expected_index] = remaining_part;
//       unpushed_data.erase( write_it );
//       break;
//     }
//   }

//   // Check if we should close the stream
//   if ( _eof && next_expected_index >= _eof_index ) {
//     output_.writer().close();
//   }
// }

void Reassembler::insert( uint64_t first_index, std::string data, bool is_last_substring )
{
  if ( is_last_substring ) {
    _eof = true;
    _eof_index = first_index + data.length();
  }

  if ( output_.writer().is_closed() )
    return;

  // Handle empty last substring
  if ( data.empty() && is_last_substring && first_index == next_expected_index ) {
    output_.writer().close();
    return;
  }

  // Skip if already written or beyond capacity
  uint64_t end_index = first_index + data.length();
  if ( end_index <= next_expected_index )
    return;

  uint64_t capacity_limit = next_expected_index + output_.writer().available_capacity();
  if ( first_index >= capacity_limit )
    return;

  // Trim data to fit within bounds
  if ( first_index < next_expected_index ) {
    data = data.substr( next_expected_index - first_index );
    first_index = next_expected_index;
  }
  if ( first_index + data.length() > capacity_limit ) {
    data = data.substr( 0, capacity_limit - first_index );
  }

  if ( data.empty() )
    return;

  // Handle overlaps by removing conflicting segments
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

  // Remove overlapping segments
  end_index = first_index + data.length();
  while ( it != unpushed_data.end() && it->first < end_index ) {
    if ( it->first + it->second.length() <= end_index ) {
      it = unpushed_data.erase( it );
    } else {
      data = data.substr( 0, it->first - first_index );
      break;
    }
  }

  // Store segment if not empty
  if ( !data.empty() ) {
    unpushed_data[first_index] = data;
  }

  // Write contiguous data
  while ( true ) {
    auto next_it = unpushed_data.find( next_expected_index );
    if ( next_it == unpushed_data.end() )
      break;

    const std::string& segment = next_it->second;
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

  // Close if EOF reached
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
