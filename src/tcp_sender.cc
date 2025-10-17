#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <algorithm> 
#include <string>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if (writer().has_error()) { // First, check if the stream is in an error state.
    transmit(make_empty_message()); // If so, send a reset segment and do nothing else.
    return;
  }

  uint64_t effective_window = ( window_size_ == 0 ) ? 1 : window_size_; // Treat a zero-sized window as size one to allow probing.

  while ( bytes_in_flight_ < effective_window ) { // Continue sending as long as there is space in the receiver's window.
    if (writer().has_error()) { // Check for error again inside the loop.
      transmit(make_empty_message());
      return;
    }
    
    uint64_t remaining_window_space = effective_window - bytes_in_flight_; // Calculate available space in the window.
    
    TCPSenderMessage msg; // Create a new message to send.
    msg.seqno = Wrap32::wrap( next_seqno_, isn_ ); // Set the sequence number for the message.

    if ( !syn_sent_ ) { // If the initial SYN hasn't been sent...
      msg.SYN = true; // ...set the SYN flag for this first segment.
      syn_sent_ = true; // Mark SYN as sent.
    }

    size_t payload_size_limit = min( TCPConfig::MAX_PAYLOAD_SIZE, remaining_window_space - msg.SYN ); // Determine payload size, respecting MAX_PAYLOAD_SIZE and window space.
    
    string_view available_data = reader().peek(); // Use peek() to view data in the stream without removing it.
    size_t actual_payload_size = min(payload_size_limit, available_data.length()); // Determine the actual amount of data to send.
    string payload_data(available_data.substr(0, actual_payload_size)); // Create a string with the payload data.
    
    msg.payload = std::move(payload_data); // Assign the payload to the message.

    reader().pop( actual_payload_size ); // Remove the data that was just sent from the ByteStream.

    if ( reader().is_finished() && !fin_sent_ && ( remaining_window_space > msg.sequence_length() ) ) { // If the stream has ended, FIN not sent, and there's room...
      msg.FIN = true; // ...set the FIN flag.
      fin_sent_ = true; // Mark FIN as sent.
    }

    if ( msg.sequence_length() == 0 ) { // If the message is empty (no SYN, FIN, or payload)...
      break; // ...stop trying to send.
    }

    if ( !timer_running_ ) { // If the timer isn't running...
      timer_running_ = true; // ...start it, since we're sending data.
      time_elapsed_ms_ = 0; // Reset the timer's elapsed time.
    }

    transmit( msg ); // Transmit the message using the provided function.

    next_seqno_ += msg.sequence_length(); // Advance the next sequence number.
    bytes_in_flight_ += msg.sequence_length(); // Increase the count of bytes in flight.
    outstanding_segments_.push( msg ); // Add the segment to the queue of outstanding segments.
    
    if ( msg.FIN ) { // If a FIN has been sent...
      break; // ...stop, as there's nothing more to send.
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg; // Create an empty message.
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ ); // Set the sequence number correctly.
  msg.RST = writer().has_error(); // Set the RST flag if the ByteStream has an error.
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST) { // **FIX**: Check if the RST flag is set on the incoming message.
    writer().set_error(); // If so, set the local ByteStream to an error state.
    return; // Abort further processing.
  }
  
  if (writer().has_error()) { // If the stream already has an error, ignore incoming messages.
    return;
  }

  window_size_ = msg.window_size; // Always update the window size from the receiver's message.

  if ( msg.ackno.has_value() ) { // Check if the message contains an acknowledgment number.
    uint64_t new_ackno_abs = msg.ackno.value().unwrap( isn_, next_seqno_ ); // Use the unwrap member function of the Wrap32 object.

    if ( new_ackno_abs > next_seqno_ ) { // If the ack is for data not yet sent...
      return; // ...it's invalid, so ignore it.
    }

    if ( new_ackno_abs > last_ackno_received_ ) { // Process only if this is an acknowledgment for new data.
      last_ackno_received_ = new_ackno_abs; // Update the highest acknowledgment number received.

      rto_ms_ = initial_RTO_ms_; // Reset the RTO to its initial value.
      consecutive_retransmissions_ = 0; // Reset the count of consecutive retransmissions.

      while ( !outstanding_segments_.empty() ) { // Iterate through outstanding segments.
        const auto& front_msg = outstanding_segments_.front(); // Get the oldest outstanding segment.
        uint64_t msg_end_abs = front_msg.seqno.unwrap( isn_, next_seqno_ ) + front_msg.sequence_length(); // Calculate its end sequence number.

        if ( msg_end_abs <= new_ackno_abs ) { // If the segment is now fully acknowledged...
          bytes_in_flight_ -= front_msg.sequence_length(); // ...decrease bytes in flight.
          outstanding_segments_.pop(); // ...and remove it from the queue.
        } else {
          break; // Stop when the first unacknowledged segment is found.
        }
      }

      if ( !outstanding_segments_.empty() ) { // If there are still outstanding segments...
        time_elapsed_ms_ = 0; // ...restart the timer.
      } else {
        timer_running_ = false; // ...otherwise, stop the timer as all data is acknowledged.
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( !timer_running_ ) { // Do nothing if the timer is not running.
    return;
  }
  
  if (writer().has_error()) { // Check for an error on each tick.
    transmit(make_empty_message());
    return;
  }

  time_elapsed_ms_ += ms_since_last_tick; // Increment the elapsed time.

  if ( time_elapsed_ms_ >= rto_ms_ ) { // Check if the retransmission timer has expired.
    if ( !outstanding_segments_.empty() ) { // If there are segments awaiting acknowledgment...
      transmit( outstanding_segments_.front() ); // ...retransmit the earliest one.
      
      if ( window_size_ > 0 ) { // If the window is not zero...
        consecutive_retransmissions_++; // ...increment the retransmission counter.
        rto_ms_ *= 2; // ...and double the RTO (exponential backoff).
      }
    }
    time_elapsed_ms_ = 0; // Restart the timer for the next RTO period.
  }
}