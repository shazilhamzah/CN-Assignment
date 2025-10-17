#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , rto_ms_( initial_RTO_ms ) // This line fixes the error.
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t rto_ms_;                // The current retransmission timeout (RTO) value in milliseconds.
  uint64_t time_elapsed_ms_ { 0 }; // Tracks the time elapsed since the retransmission timer was last reset.
  bool timer_running_ { false };   // A flag indicating if the retransmission timer is active.
  uint64_t next_seqno_ { 0 };      // The absolute sequence number of the next byte to be sent.
  std::queue<TCPSenderMessage>
    outstanding_segments_ {}; // A queue of segments that have been sent but not yet acknowledged
  uint64_t bytes_in_flight_ {
    0 }; // The total number of sequence numbers currently in flight (sent but not acknowledged).
  uint64_t consecutive_retransmissions_ { 0 }; // A counter for the number of consecutive retransmissions.
  uint16_t window_size_ { 1 };                 // The receiver's advertised window size.
  uint64_t last_ackno_received_ { 0 };         // The highest absolute acknowledgment number received from the peer.
  bool syn_sent_ { false };                    // A flag to ensure the SYN segment is sent only once.
  bool fin_sent_ { false };                    // A flag to ensure the FIN segment is sent only once.
};
