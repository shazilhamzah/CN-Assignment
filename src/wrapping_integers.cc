#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

//
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) // convert abs number to wrapped number
{

  return Wrap32 { zero_point.raw_value_
                  + static_cast<uint32_t>(
                    n ) }; // here using <uint32_t> is the replacement of n % 2^32, as it takes the lowest bits of
                           // n. Then just add the zero_point value so the sequence number start from the ISN not 0.
                           // If the result is larger than 2^32, it is automatically wrapped around.

  // if n is 4294967400 (which is 2^32 + 104), and our zero point is 296, then uint32 wraps around to 104, hence the
  // wrapped number is 104 + 296 = 400. This is basically the logic behind the wrap function. Similar to a clock,
  // where after 12 pm, it wraps to 0 (technically 1) but the absolute time is different
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{

  uint64_t modValue = static_cast<uint64_t>( 1 )
                      << 32; // <<32 is using the bitwise left shift operator, it is the same as multiplying with
                             // 2^32, the idea is that wraparounds occur every 2^32 so I have stored that.

  uint64_t offsetValue
    = raw_value_
      - zero_point
          .raw_value_; // this gives us the offset (seqno (the number we want to convert back) - zero point (initial
                       // sequence number ISN - starting point)) -- how many numbers ahead from the ISN

  uint64_t combinedValue
    = ( checkpoint & ~( static_cast<uint64_t>( 0xFFFFFFFF ) ) )
      | static_cast<uint64_t>(
        offsetValue ); // setting the offsetValue in the lower 32 bits and checkpoint's upper 32 bits

  if ( combinedValue + ( modValue / 2 )
       < checkpoint ) { // if checkpoint value is greater than what we need, then go forward 1 wrap
    combinedValue += modValue;
  }

  else if ( combinedValue > checkpoint + ( modValue / 2 )
            && combinedValue
                 >= modValue ) { // if checkpoint value is less than what we need, then go back 1 wrap, the code was
                                 // failing the test 18 as I didn't address the underflow situation, hence the
                                 // combinedValue >= modValue condition to ensure a non-negative value

    combinedValue -= modValue;
  }

  return { combinedValue }; // closet abs sequence number that matches the wrapped seqno
}
