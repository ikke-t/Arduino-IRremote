#include "IRremote.h"
#include "IRremoteInt.h"

//==============================================================================
//
//
//                           T O S H I B A
//
//  Toshiba Heat Pump has a remote WH-H07JE. It sends three different lenghts
//  signals. Most of the buttons go within 9 byte sequence. The last byte is
//  some kind of CRC. Usage of results->data is questionable, as the bit
//  sequences won't fit into long type variable. You can use that to identify
//  signal lenght, the value is different for different lenght signals.
//
//  Real signal data is readable from raw data using the example bit structures.
//
//  TBD: Send is not tested, as I don't have the receiver now present. There is
//  some CRC in last byte that needs to be figured out first.
//==============================================================================

#define TOSHIBA_BITS_1        72
#define TOSHIBA_BYTES_1        9
#define TOSHIBA_BITS_2        56
#define TOSHIBA_BYTES_2        7
#define TOSHIBA_BITS_3        80
#define TOSHIBA_BYTES_3       10
#define TOSHIBA_HDR_MARK    4400
#define TOSHIBA_HDR_SPACE   4400
#define TOSHIBA_BIT_MARK     550
#define TOSHIBA_ONE_SPACE   1600
#define TOSHIBA_ZERO_SPACE   550

// Bit structure of the heat pump command structure.
//
typedef union cmd {
  byte bytes[TOSHIBA_BYTES_1];
  struct bits {
    byte byte0;        // always F2
    byte byte1;        // always 0D
    byte byte2;        // always 03
    byte byte3;        // always FC
    byte byte4;        // always 01
    // byte 5
    byte temp   :4;    // 17-30 degrees, value as: celsius - 17 = 0-13
    byte byte5  :4;    // unused
    // byte 6
    byte fan    :3;    // 0=auto, 2=*, 3=**, 4=***, 5=****, 6=*****  
    byte mode   :2;    // 0=auto, 1=cool, 2=dry, 3=heat
    byte power  :3;    // 0=on 7=off
    // byte 7
    byte byte71 :3;    // unused
    byte pure   :1;    // 1=on, 0=off  
    byte byte72 :4;    // unused
    // byte 8
    byte crc    :8;    // some sort of CRC
  };
};

//+=============================================================================
#if SEND_TOSHIBA
void  IRsend::sendTOSHIBA (unsigned long data,  int nbits)
{
	// Set IR carrier frequency
	enableIROut(38);

	// Header
	mark(TOSHIBA_HDR_MARK);
	space(TOSHIBA_HDR_SPACE);

	// Data
	for (unsigned long  mask = 1UL << (nbits - 1);  mask;  mask >>= 1) {
		if (data & mask) {
			mark(TOSHIBA_BIT_MARK);
			space(TOSHIBA_ONE_SPACE);
		} else {
			mark(TOSHIBA_BIT_MARK);
			space(TOSHIBA_ZERO_SPACE);
		}
	}

	// Footer
	mark(TOSHIBA_BIT_MARK);
    space(0);  // Always end with the LED off
}
#endif

#if DECODE_TOSHIBA
bool  IRrecv::decodeTOSHIBA (decode_results *results)
{
	long  data   = 0;
	int   offset = 1;  // Skip first space
	int   bits   = 0;
	int   bytes  = 0;

	// Initial mark
	if (!MATCH_MARK(results->rawbuf[offset++], TOSHIBA_HDR_MARK))   return false ;

	bits = (results->rawlen - 3)/2;
	if (!(bits == TOSHIBA_BITS_1 ||
		bits == TOSHIBA_BITS_2 ||
		bits == TOSHIBA_BITS_3)) {
		return false ;
	}
	bytes = bits/8;

	// Initial space
	if (!MATCH_SPACE(results->rawbuf[offset++], TOSHIBA_HDR_SPACE))  return false ;

    // we only dump 32 bits as it all won't fit into such small variable
	for (int i = 0;  i < 32;   i++) {
		if (!MATCH_MARK(results->rawbuf[offset++], TOSHIBA_BIT_MARK))  return false ;

		if      (MATCH_SPACE(results->rawbuf[offset], TOSHIBA_ONE_SPACE))   data = (data << 1) | 1 ;
		else if (MATCH_SPACE(results->rawbuf[offset], TOSHIBA_ZERO_SPACE))  data = (data << 1) | 0 ;
		else                                                                return false ;
		offset++;
	}

	// Success
	results->bits        = bits;
	results->value       = data;
	results->decode_type = TOSHIBA;
	return true;
}
#endif

