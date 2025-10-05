#include "tcp_receiver.hh"
#include "debug.hh"

#include <limits>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage m )
{
  if ( m.RST ) {     // checking if the segment has a reset flag set
    rst_seen = true; // the goal is to set this to true so it the "send function" will keep this
    const_cast<Writer&>( reassembler_.writer() ).set_error(); // set the byte stream as errored
    return; // as the reset flag has been set to stop processing the segment
  }

  if ( !isnExists ) {
    if ( !m.SYN ) // if the ISN doesn't exist and SYN does not exist either than just ignore the segments
      return;

    isnExists = true; // else, ISN does exist/ we have SYN
    ISN = m.seqno; // store the ISN value, this is the zero point for wrap and unwrap function used for calculations
                   // of abs
  }

  uint64_t next = reassembler_.writer().bytes_pushed(); // how many bytes have been pushed
  uint64_t checkPoint = next + 1;                       // the checkpoint
  uint64_t seg_abs
    = m.seqno.unwrap( ISN, checkPoint ); // convert 32 bit seq no into 64 bit one that is close to checkpoint
  uint64_t first_abs = seg_abs + ( m.SYN ? 1 : 0 );

  reassembler_.insert(
    first_abs - 1, m.payload, m.FIN ); // insert the payload at the stream index - 1, while passing the finish flag
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage out;

  uint64_t availableCap = reassembler_.writer().available_capacity(); // the availble capacity

  if ( availableCap > static_cast<uint64_t>( numeric_limits<uint16_t>::max() ) ) {
    out.window_size = numeric_limits<uint16_t>::max(); // clamp to max value of 16-bit
  }

  else {
    out.window_size = static_cast<uint16_t>( availableCap );
  }

  if ( isnExists ) { // if the ISN exists then provide a ack number
    uint64_t abs_ack = reassembler_.writer().bytes_pushed() + 1;

    if ( reassembler_.writer().is_closed() )
      abs_ack += 1;

    out.ackno = Wrap32::wrap( abs_ack, ISN );
  }

  out.RST = ( rst_seen || reassembler_.writer().has_error() );
  return out;
}
