// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// mode_s.c: Mode S message decoding.
//
// Copyright (c) 2014,2015 Oliver Jowett <oliver@mutability.co.uk>
//
// This file is free software: you may copy, redistribute and/or modify it  
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 2 of the License, or (at your  
// option) any later version.  
//
// This file is distributed in the hope that it will be useful, but  
// WITHOUT ANY WARRANTY; without even the implied warranty of  
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License  
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// This file incorporates work covered by the following copyright and  
// permission notice:
//
//   Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
//
//   All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are
//   met:
//
//    *  Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//    *  Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "dump1090.h"

/* for PRIX64 */
#include <inttypes.h>

//
// ===================== Mode S detection and decoding  ===================
//
//
//

/* A timestamp that indicates the data is synthetic, created from a
 * multilateration result
 */
#define MAGIC_MLAT_TIMESTAMP 0xFF004D4C4154ULL

//=========================================================================
//
// Given the Downlink Format (DF) of the message, return the message length in bits.
//
// All known DF's 16 or greater are long. All known DF's 15 or less are short. 
// There are lots of unused codes in both category, so we can assume ICAO will stick to 
// these rules, meaning that the most significant bit of the DF indicates the length.
//
int modesMessageLenByType(int type) {
    return (type & 0x10) ? MODES_LONG_MSG_BITS : MODES_SHORT_MSG_BITS ;
}

//
//=========================================================================
//
// In the squawk (identity) field bits are interleaved as follows in
// (message bit 20 to bit 32):
//
// C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
//
// So every group of three bits A, B, C, D represent an integer from 0 to 7.
//
// The actual meaning is just 4 octal numbers, but we convert it into a hex 
// number tha happens to represent the four octal numbers.
//
// For more info: http://en.wikipedia.org/wiki/Gillham_code
//
static int decodeID13Field(int ID13Field) {
    int hexGillham = 0;

    if (ID13Field & 0x1000) {hexGillham |= 0x0010;} // Bit 12 = C1
    if (ID13Field & 0x0800) {hexGillham |= 0x1000;} // Bit 11 = A1
    if (ID13Field & 0x0400) {hexGillham |= 0x0020;} // Bit 10 = C2
    if (ID13Field & 0x0200) {hexGillham |= 0x2000;} // Bit  9 = A2
    if (ID13Field & 0x0100) {hexGillham |= 0x0040;} // Bit  8 = C4
    if (ID13Field & 0x0080) {hexGillham |= 0x4000;} // Bit  7 = A4
  //if (ID13Field & 0x0040) {hexGillham |= 0x0800;} // Bit  6 = X  or M 
    if (ID13Field & 0x0020) {hexGillham |= 0x0100;} // Bit  5 = B1 
    if (ID13Field & 0x0010) {hexGillham |= 0x0001;} // Bit  4 = D1 or Q
    if (ID13Field & 0x0008) {hexGillham |= 0x0200;} // Bit  3 = B2
    if (ID13Field & 0x0004) {hexGillham |= 0x0002;} // Bit  2 = D2
    if (ID13Field & 0x0002) {hexGillham |= 0x0400;} // Bit  1 = B4
    if (ID13Field & 0x0001) {hexGillham |= 0x0004;} // Bit  0 = D4

    return (hexGillham);
}

#define INVALID_ALTITUDE (-9999)

//
//=========================================================================
//
// Decode the 13 bit AC altitude field (in DF 20 and others).
// Returns the altitude, and set 'unit' to either MODES_UNIT_METERS or MDOES_UNIT_FEETS.
//
static int decodeAC13Field(int AC13Field, int *unit) {
    int m_bit  = AC13Field & 0x0040; // set = meters, clear = feet
    int q_bit  = AC13Field & 0x0010; // set = 25 ft encoding, clear = Gillham Mode C encoding

    if (!m_bit) {
        *unit = MODES_UNIT_FEET;
        if (q_bit) {
            // N is the 11 bit integer resulting from the removal of bit Q and M
            int n = ((AC13Field & 0x1F80) >> 2) |
                    ((AC13Field & 0x0020) >> 1) |
                     (AC13Field & 0x000F);
            // The final altitude is resulting number multiplied by 25, minus 1000.
            return ((n * 25) - 1000);
        } else {
            // N is an 11 bit Gillham coded altitude
            int n = ModeAToModeC(decodeID13Field(AC13Field));
            if (n < -12) {
                return INVALID_ALTITUDE;
            }

            return (100 * n);
        }
    } else {
        *unit = MODES_UNIT_METERS;
        // TODO: Implement altitude when meter unit is selected
        return INVALID_ALTITUDE;
    }
}
//
//=========================================================================
//
// Decode the 12 bit AC altitude field (in DF 17 and others).
//
static int decodeAC12Field(int AC12Field, int *unit) {
    int q_bit  = AC12Field & 0x10; // Bit 48 = Q

    *unit = MODES_UNIT_FEET;
    if (q_bit) {
        /// N is the 11 bit integer resulting from the removal of bit Q at bit 4
        int n = ((AC12Field & 0x0FE0) >> 1) | 
                 (AC12Field & 0x000F);
        // The final altitude is the resulting number multiplied by 25, minus 1000.
        return ((n * 25) - 1000);
    } else {
        // Make N a 13 bit Gillham coded altitude by inserting M=0 at bit 6
        int n = ((AC12Field & 0x0FC0) << 1) | 
                 (AC12Field & 0x003F);
        n = ModeAToModeC(decodeID13Field(n));
        if (n < -12) {
            return INVALID_ALTITUDE;
        }

        return (100 * n);
    }
}
//
//=========================================================================
//
// Decode the 7 bit ground movement field PWL exponential style scale
//
static int decodeMovementField(int movement) {
    int gspeed;

    // Note : movement codes 0,125,126,127 are all invalid, but they are 
    //        trapped for before this function is called.

    if      (movement  > 123) gspeed = 199; // > 175kt
    else if (movement  > 108) gspeed = ((movement - 108)  * 5) + 100;
    else if (movement  >  93) gspeed = ((movement -  93)  * 2) +  70;
    else if (movement  >  38) gspeed = ((movement -  38)     ) +  15;
    else if (movement  >  12) gspeed = ((movement -  11) >> 1) +   2;
    else if (movement  >   8) gspeed = ((movement -   6) >> 2) +   1;
    else                      gspeed = 0;

    return (gspeed);
}
//
//=========================================================================
//
// Capability table
static const char *ca_str[8] = {
    /* 0 */ "Level 1",
    /* 1 */ "reserved",
    /* 2 */ "reserved",
    /* 3 */ "reserved",
    /* 4 */ "Level 2+, ground",
    /* 5 */ "Level 2+, airborne",
    /* 6 */ "Level 2+",
    /* 7 */ "DR/Alert/SPI active"
};

// DF 18 Control field table.
static const char *cf_str[8] = {
    /* 0 */ "ADS-B ES/NT device with ICAO 24-bit address",
    /* 1 */ "ADS-B ES/NT device with other address",
    /* 2 */ "Fine format TIS-B",
    /* 3 */ "Coarse format TIS-B",
    /* 4 */ "TIS-B management message",
    /* 5 */ "TIS-B relay of ADS-B message with other address",
    /* 6 */ "ADS-B rebroadcast using DF-17 message format",
    /* 7 */ "Reserved"
};

// Flight status table
static const char *fs_str[8] = {
    /* 0 */ "Normal, Airborne",
    /* 1 */ "Normal, On the ground",
    /* 2 */ "ALERT,  Airborne",
    /* 3 */ "ALERT,  On the ground",
    /* 4 */ "ALERT & Special Position Identification. Airborne or Ground",
    /* 5 */ "Special Position Identification. Airborne or Ground",
    /* 6 */ "Reserved",
    /* 7 */ "Not assigned"
};

// Emergency state table
// from https://www.ll.mit.edu/mission/aviation/publications/publication-files/atc-reports/Grappel_2007_ATC-334_WW-15318.pdf
// and 1090-DO-260B_FRAC
char *es_str[8] = {
    /* 0 */ "No emergency",
    /* 1 */ "General emergency (squawk 7700)",
    /* 2 */ "Lifeguard/Medical",
    /* 3 */ "Minimum fuel",
    /* 4 */ "No communications (squawk 7600)",
    /* 5 */ "Unlawful interference (squawk 7500)",
    /* 6 */ "Reserved",
    /* 7 */ "Reserved"
};
//
//=========================================================================
//
static char *getMEDescription(int metype, int mesub) {
    char *mename = "Unknown";

    if (metype >= 1 && metype <= 4)
        mename = "Aircraft Identification and Category";
    else if (metype >= 5 && metype <= 8)
        mename = "Surface Position";
    else if (metype >= 9 && metype <= 18)
        mename = "Airborne Position (Baro Altitude)";
    else if (metype == 19 && mesub >=1 && mesub <= 4)
        mename = "Airborne Velocity";
    else if (metype >= 20 && metype <= 22)
        mename = "Airborne Position (GNSS Height)";
    else if (metype == 23 && mesub == 0)
        mename = "Test Message";
    else if (metype == 23 && mesub == 7)
        mename = "Test Message -- Squawk";
    else if (metype == 24 && mesub == 1)
        mename = "Surface System Status";
    else if (metype == 28 && mesub == 1)
        mename = "Extended Squitter Aircraft Status (Emergency)";
    else if (metype == 28 && mesub == 2)
        mename = "Extended Squitter Aircraft Status (1090ES TCAS RA)";
    else if (metype == 29 && (mesub == 0 || mesub == 1))
        mename = "Target State and Status Message";
    else if (metype == 31 && (mesub == 0 || mesub == 1))
        mename = "Aircraft Operational Status Message";
    return mename;
}

// Correct a decoded native-endian Address Announced field
// (from bits 8-31) if it is affected by the given error
// syndrome. Updates *addr and returns >0 if changed, 0 if
// it was unaffected.
static int correct_aa_field(uint32_t *addr, struct errorinfo *ei) 
{
    int i;
    int addr_errors = 0;

    if (!ei)
        return 0;

    for (i = 0; i < ei->errors; ++i) {
        if (ei->bit[i] >= 8 && ei->bit[i] <= 31) {
            *addr ^= 1 << (31 - ei->bit[i]);
            ++addr_errors;
        }
    }

    return addr_errors;
}

// Score how plausible this ModeS message looks.
// The more positive, the more reliable the message is

// 1000: DF 0/4/5/16/24 with a CRC-derived address matching a known aircraft

// 1800: DF17/18 with good CRC and an address matching a known aircraft
// 1400: DF17/18 with good CRC and an address not matching a known aircraft
//  900: DF17/18 with 1-bit error and an address matching a known aircraft
//  700: DF17/18 with 1-bit error and an address not matching a known aircraft
//  450: DF17/18 with 2-bit error and an address matching a known aircraft
//  350: DF17/18 with 2-bit error and an address not matching a known aircraft

// 1600: DF11 with IID==0, good CRC and an address matching a known aircraft
//  800: DF11 with IID==0, 1-bit error and an address matching a known aircraft
//  750: DF11 with IID==0, good CRC and an address not matching a known aircraft
//  375: DF11 with IID==0, 1-bit error and an address not matching a known aircraft

// 1000: DF11 with IID!=0, good CRC and an address matching a known aircraft
//  500: DF11 with IID!=0, 1-bit error and an address matching a known aircraft

// 1000: DF20/21 with a CRC-derived address matching a known aircraft
//  500: DF20/21 with a CRC-derived address matching a known aircraft (bottom 16 bits only - overlay control in use)

//   -1: message might be valid, but we couldn't validate the CRC against a known ICAO
//   -2: bad message or unrepairable CRC error

int scoreModesMessage(unsigned char *msg, int validbits)
{
    int msgtype, msgbits, crc, iid;
    uint32_t addr;
    struct errorinfo *ei;
    static unsigned char all_zeros[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (validbits < 56)
        return -2;

    msgtype = msg[0] >> 3; // Downlink Format
    msgbits = modesMessageLenByType(msgtype);

    if (validbits < msgbits)
        return -2;

    if (!memcmp(all_zeros, msg, msgbits/8))
        return -2;

    crc = modesChecksum(msg, msgbits);

    switch (msgtype) {
    case 0: // short air-air surveillance
    case 4: // surveillance, altitude reply
    case 5: // surveillance, altitude reply
    case 16: // long air-air surveillance
    case 24: // Comm-D (ELM)
        return icaoFilterTest(crc) ? 1000 : -1;

    case 11: // All-call reply
        iid = crc & 0x7f;
        crc = crc & 0xffff80;
        addr = (msg[1] << 16) | (msg[2] << 8) | (msg[3]);

        ei = modesChecksumDiagnose(crc, msgbits);
        if (!ei)
            return -2; // can't correct errors

        // see crc.c comments: we do not attempt to fix
        // more than single-bit errors, as two-bit
        // errors are ambiguous in DF11.
        if (ei->errors > 1)
            return -2; // can't correct errors

        // fix any errors in the address field
        correct_aa_field(&addr, ei);

        // validate address
        if (iid == 0) {
            if (icaoFilterTest(addr))
                return 1600 / (ei->errors + 1);
            else
                return 750 / (ei->errors + 1);
        } else {
            if (icaoFilterTest(addr))
                return 1000 / (ei->errors + 1);
            else
                return -1;
        }
        
    case 17:   // Extended squitter
    case 18:   // Extended squitter/non-transponder
        ei = modesChecksumDiagnose(crc, msgbits);
        if (!ei)
            return -2; // can't correct errors

        // fix any errors in the address field
        addr = (msg[1] << 16) | (msg[2] << 8) | (msg[3]);
        correct_aa_field(&addr, ei);        

        if (icaoFilterTest(addr))
            return 1800 / (ei->errors+1);
        else
            return 1400 / (ei->errors+1);

    case 20:   // Comm-B, altitude reply
    case 21:   // Comm-B, identity reply
        if (icaoFilterTest(crc))
            return 1000; // Address/Parity

#if 0
        // This doesn't seem useful, as we mistake a lot of CRC errors
        // for overlay control
        if (icaoFilterTestFuzzy(crc))
            return 500;  // Data/Parity
#endif

        return -2;

    default:
        // unknown message type
        return -2;
    }
}

//
//=========================================================================
//
// Decode a raw Mode S message demodulated as a stream of bytes by detectModeS(), 
// and split it into fields populating a modesMessage structure.
//

static void decodeExtendedSquitter(struct modesMessage *mm);
static void decodeCommB(struct modesMessage *mm);
static char *ais_charset = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?";

// return 0 if all OK
//   -1: message might be valid, but we couldn't validate the CRC against a known ICAO
//   -2: bad message or unrepairable CRC error

int decodeModesMessage(struct modesMessage *mm, unsigned char *msg)
{
    // Work on our local copy.
    memcpy(mm->msg, msg, MODES_LONG_MSG_BYTES);
    if (Modes.net_verbatim) {
        // Preserve the original uncorrected copy for later forwarding
        memcpy(mm->verbatim, msg, MODES_LONG_MSG_BYTES);
    }
    msg = mm->msg;

    // Get the message type ASAP as other operations depend on this
    mm->msgtype         = msg[0] >> 3; // Downlink Format
    mm->msgbits         = modesMessageLenByType(mm->msgtype);
    mm->crc             = modesChecksum(msg, mm->msgbits);
    mm->correctedbits   = 0;
    mm->addr            = 0;

    // Do checksum work and set fields that depend on the CRC
    switch (mm->msgtype) {
    case 0: // short air-air surveillance
    case 4: // surveillance, altitude reply
    case 5: // surveillance, altitude reply
    case 16: // long air-air surveillance
    case 24: // Comm-D (ELM)
        // These message types use Address/Parity, i.e. our CRC syndrome is the sender's ICAO address.
        // We can't tell if the CRC is correct or not as we don't know the correct address.
        // Accept the message if it appears to be from a previously-seen aircraft
        if (!icaoFilterTest(mm->crc)) {
           return -1;
        }
        mm->addr = mm->crc;
        break;

    case 11: // All-call reply
        // This message type uses Parity/Interrogator, i.e. our CRC syndrome is CL + IC from the uplink message
        // which we can't see. So we don't know if the CRC is correct or not.
        //
        // however! CL + IC only occupy the lower 7 bits of the CRC. So if we ignore those bits when testing
        // the CRC we can still try to detect/correct errors.

        mm->iid   =  mm->crc & 0x7f;
        if (mm->crc & 0xffff80) {
            int addr;
            struct errorinfo *ei = modesChecksumDiagnose(mm->crc & 0xffff80, mm->msgbits);
            if (!ei) {
                return -2; // couldn't fix it
            }

            // see crc.c comments: we do not attempt to fix
            // more than single-bit errors, as two-bit
            // errors are ambiguous in DF11.
            if (ei->errors > 1)
                return -2; // can't correct errors

            mm->correctedbits = ei->errors;
            modesChecksumFix(msg, ei);

            // check whether the corrected message looks sensible
            // we are conservative here: only accept corrected messages that
            // match an existing aircraft.
            addr = (msg[1] << 16) | (msg[2] << 8) | (msg[3]); 
            if (!icaoFilterTest(addr)) {
                return -1;
            }
        }
        break;

    case 17:   // Extended squitter
    case 18: { // Extended squitter/non-transponder
        struct errorinfo *ei;
        int addr1, addr2;

        // These message types use Parity/Interrogator, but are specified to set II=0

        if (mm->crc == 0)
            break;  // all good

        ei = modesChecksumDiagnose(mm->crc, mm->msgbits);
        if (!ei) {
            return -2; // couldn't fix it
        }

        addr1 = (msg[1] << 16) | (msg[2] << 8) | (msg[3]); 
        mm->correctedbits = ei->errors;
        modesChecksumFix(msg, ei);
        addr2 = (msg[1] << 16) | (msg[2] << 8) | (msg[3]); 
        
        // we are conservative here: only accept corrected messages that
        // match an existing aircraft.
        if (addr1 != addr2 && !icaoFilterTest(addr2)) {
            return -1;
        }

        break;
    }

    case 20: // Comm-B, altitude reply
    case 21: // Comm-B, identity reply
        // These message types either use Address/Parity (see DF0 etc)
        // or Data Parity where the requested BDS is also xored into the top byte.
        // So not only do we not know whether the CRC is right, we also don't know if
        // the ICAO is right! Ow.

        // Try an exact match
        if (icaoFilterTest(mm->crc)) {
            // OK.
            mm->addr = mm->crc;
            mm->bds = 0; // unknown
            break;
        }

#if 0
        // This doesn't seem useful, as we mistake a lot of CRC errors
        // for overlay control

        // Try a fuzzy match
        if ( (mm->addr = icaoFilterTestFuzzy(mm->crc)) != 0) {
            // We have an address that would match, assume it's correct
            mm->bds = (mm->crc ^ mm->addr) >> 16; // derive the BDS value based on what we think the address is
            break;
        }
#endif

        return -1; // no good

    default:
        // All other message types, we don't know how to handle their CRCs, give up
        return -2;
    }      

    // decode the bulk of the message

    mm->bFlags = 0;

    if (mm->remote && mm->timestampMsg == MAGIC_MLAT_TIMESTAMP)
        mm->bFlags |= MODES_ACFLAGS_FROM_MLAT;

    // AA (Address announced)
    if (mm->msgtype == 11 || mm->msgtype == 17 || mm->msgtype == 18) {
        mm->addr  = (msg[1] << 16) | (msg[2] << 8) | (msg[3]);
    }

    // AC (Altitude Code)
    if (mm->msgtype == 0 || mm->msgtype == 4 || mm->msgtype == 16 || mm->msgtype == 20) {
        int AC13Field = ((msg[2] << 8) | msg[3]) & 0x1FFF; 
        if (AC13Field) { // Only attempt to decode if a valid (non zero) altitude is present
            mm->altitude = decodeAC13Field(AC13Field, &mm->unit);
            if (mm->altitude != INVALID_ALTITUDE)
                mm->bFlags  |= MODES_ACFLAGS_ALTITUDE_VALID;
        }
    }

    // AF (DF19 Application Field) not decoded

    // CA (Capability)
    if (mm->msgtype == 11 || mm->msgtype == 17) {
        mm->ca    = (msg[0] & 0x07);
        if (mm->ca == 4) {
            mm->bFlags |= MODES_ACFLAGS_AOG_VALID | MODES_ACFLAGS_AOG;
        } else if (mm->ca == 5) {
            mm->bFlags |= MODES_ACFLAGS_AOG_VALID;
        }
    }

    // CC (Cross-link capability) not decoded

    // CF (Control field)
    if (mm->msgtype == 18) {
        mm->cf = msg[0] & 7;
    }

    // DR (Downlink Request) not decoded

    // FS (Flight Status)
    if (mm->msgtype == 4 || mm->msgtype == 5 || mm->msgtype == 20 || mm->msgtype == 21) {
        mm->bFlags  |= MODES_ACFLAGS_FS_VALID;
        mm->fs = msg[0] & 7;
        if (mm->fs <= 3) {
            mm->bFlags |= MODES_ACFLAGS_AOG_VALID;
            if (mm->fs & 1)
                mm->bFlags |= MODES_ACFLAGS_AOG;
        }
    }

    // ID (Identity)
    if (mm->msgtype == 5  || mm->msgtype == 21) {
        // Gillham encoded Squawk
        int ID13Field = ((msg[2] << 8) | msg[3]) & 0x1FFF; 
        if (ID13Field) {
            mm->bFlags |= MODES_ACFLAGS_SQUAWK_VALID;
            mm->modeA   = decodeID13Field(ID13Field);
        }
    }

    // KE (Control, ELM) not decoded

    // MB (messsage, Comm-B)
    if (mm->msgtype == 20 || mm->msgtype == 21) {
        decodeCommB(mm);
    }

    // MD (message, Comm-D) not decoded

    // ME (message, extended squitter)
    if (mm->msgtype == 17 ||   //  Extended squitter
        mm->msgtype == 18) {   //  Extended squitter/non-transponder:
        decodeExtendedSquitter(mm);
    }

    // MV (message, ACAS) not decoded
    // ND (number of D-segment) not decoded
    // RI (Reply information) not decoded
    // SL (Sensitivity level, ACAS) not decoded
    // UM (Utility Message) not decoded

    // VS (Vertical Status)
    if (mm->msgtype == 0 || mm->msgtype == 16) {
        mm->bFlags |= MODES_ACFLAGS_AOG_VALID;        
        if (msg[0] & 0x04)
            mm->bFlags |= MODES_ACFLAGS_AOG;
    }

    if (!mm->correctedbits && (mm->msgtype == 17 || mm->msgtype == 18 || (mm->msgtype == 11 && mm->iid == 0))) {
        // No CRC errors seen, and either it was an DF17/18 extended squitter
        // or a DF11 acquisition squitter with II = 0. We probably have the right address.

        // We wait until here to do this as we may have needed to decode an ES to note
        // the type of address in DF18 messages.

        // NB this is the only place that adds addresses!
        icaoFilterAdd(mm->addr);
    }

    // all done
    return 0;
}

// Decode BDS2,0 carried in Comm-B or ES
static void decodeBDS20(struct modesMessage *mm)
{
    uint32_t chars1, chars2;
    unsigned char *msg = mm->msg;
    
    chars1 = (msg[5] << 16) | (msg[6] << 8) | (msg[7]);
    chars2 = (msg[8] << 16) | (msg[9] << 8) | (msg[10]);
    
    // A common failure mode seems to be to intermittently send
    // all zeros. Catch that here.
    if (chars1 == 0 && chars2 == 0)
        return;

    mm->bFlags |= MODES_ACFLAGS_CALLSIGN_VALID;
    
    mm->flight[3] = ais_charset[chars1 & 0x3F]; chars1 = chars1 >> 6;
    mm->flight[2] = ais_charset[chars1 & 0x3F]; chars1 = chars1 >> 6;
    mm->flight[1] = ais_charset[chars1 & 0x3F]; chars1 = chars1 >> 6;
    mm->flight[0] = ais_charset[chars1 & 0x3F];
    
    mm->flight[7] = ais_charset[chars2 & 0x3F]; chars2 = chars2 >> 6;
    mm->flight[6] = ais_charset[chars2 & 0x3F]; chars2 = chars2 >> 6;
    mm->flight[5] = ais_charset[chars2 & 0x3F]; chars2 = chars2 >> 6;
    mm->flight[4] = ais_charset[chars2 & 0x3F];
    
    mm->flight[8] = '\0';
}

static void decodeExtendedSquitter(struct modesMessage *mm)
{    
    unsigned char *msg = mm->msg;
    int metype = mm->metype = msg[4] >> 3;   // Extended squitter message type
    int mesub  = mm->mesub  = (metype == 29 ? ((msg[4]&6)>>1) : (msg[4]  & 7));   // Extended squitter message subtype

    int check_imf = 0;

    // Check CF on DF18 to work out the format of the ES and whether we need to look for an IMF bit
    if (mm->msgtype == 18) {
        switch (mm->cf) {
        case 0: //   ADS-B ES/NT devices that report the ICAO 24-bit address in the AA field
            break;

        case 1: //   Reserved for ADS-B for ES/NT devices that use other addressing techniques in the AA field
        case 5: //   TIS-B messages that relay ADS-B Messages using anonymous 24-bit addresses (format not explicitly defined, but it seems to follow DF17)
            mm->addr |= MODES_NON_ICAO_ADDRESS;
            break;

        case 2: //   Fine TIS-B message (formats are close enough to DF17 for our purposes)
        case 6: //   ADS-B rebroadcast using the same type codes and message formats as defined for DF = 17 ADS-B messages
            check_imf = 1;
            break;

        case 3: //   Coarse TIS-B airborne position and velocity.
            // TODO: decode me.
            // For now we only look at the IMF bit.
            if (msg[4] & 0x80)
                mm->addr |= MODES_NON_ICAO_ADDRESS;
            return;

        default:    // All others, we don't know the format.
            mm->addr |= MODES_NON_ICAO_ADDRESS; // assume non-ICAO
            return;
        }
    }



    switch (metype) {
    case 1: case 2: case 3: case 4: {
        // Aircraft Identification and Category
        uint32_t chars1, chars2;

        chars1 = (msg[5] << 16) | (msg[6] << 8) | (msg[7]);
        chars2 = (msg[8] << 16) | (msg[9] << 8) | (msg[10]);

        // A common failure mode seems to be to intermittently send
        // all zeros. Catch that here.
        if (chars1 != 0 || chars2 != 0) {
            mm->bFlags |= MODES_ACFLAGS_CALLSIGN_VALID;

            mm->flight[3] = ais_charset[chars1 & 0x3F]; chars1 = chars1 >> 6;
            mm->flight[2] = ais_charset[chars1 & 0x3F]; chars1 = chars1 >> 6;
            mm->flight[1] = ais_charset[chars1 & 0x3F]; chars1 = chars1 >> 6;
            mm->flight[0] = ais_charset[chars1 & 0x3F];
        
            mm->flight[7] = ais_charset[chars2 & 0x3F]; chars2 = chars2 >> 6;
            mm->flight[6] = ais_charset[chars2 & 0x3F]; chars2 = chars2 >> 6;
            mm->flight[5] = ais_charset[chars2 & 0x3F]; chars2 = chars2 >> 6;
            mm->flight[4] = ais_charset[chars2 & 0x3F];
        
            mm->flight[8] = '\0';
        }

        mm->category = ((0x0E - metype) << 4) | mesub;
        mm->bFlags |= MODES_ACFLAGS_CATEGORY_VALID;

        break;
    }

    case 19: { // Airborne Velocity Message        
        if (check_imf && (msg[5] & 0x80))
            mm->addr |= MODES_NON_ICAO_ADDRESS;

        // Presumably airborne if we get an Airborne Velocity Message
        mm->bFlags |= MODES_ACFLAGS_AOG_VALID; 
        
        if ( (mesub >= 1) && (mesub <= 4) ) {
            int vert_rate = ((msg[8] & 0x07) << 6) | (msg[9] >> 2);
            if (vert_rate) {
                --vert_rate;
                if (msg[8] & 0x08) 
                    {vert_rate = 0 - vert_rate;}
                mm->vert_rate =  vert_rate * 64;
                mm->bFlags   |= MODES_ACFLAGS_VERTRATE_VALID;
            }
        }

        if ((mesub == 1) || (mesub == 2)) {
            int ew_raw = ((msg[5] & 0x03) << 8) |  msg[6];
            int ew_vel = ew_raw - 1;
            int ns_raw = ((msg[7] & 0x7F) << 3) | (msg[8] >> 5);
            int ns_vel = ns_raw - 1;
            
            if (mesub == 2) { // If (supersonic) unit is 4 kts
                ns_vel = ns_vel << 2;
                ew_vel = ew_vel << 2;
            }
            
            if (ew_raw) { // Do East/West  
                mm->bFlags |= MODES_ACFLAGS_EWSPEED_VALID;
                if (msg[5] & 0x04)
                    {ew_vel = 0 - ew_vel;}                   
                mm->ew_velocity = ew_vel;
            }
            
            if (ns_raw) { // Do North/South
                mm->bFlags |= MODES_ACFLAGS_NSSPEED_VALID;
                if (msg[7] & 0x80)
                    {ns_vel = 0 - ns_vel;}                   
                mm->ns_velocity = ns_vel;
            }
            
            if (ew_raw && ns_raw) {
                // Compute velocity and angle from the two speed components
                mm->bFlags |= (MODES_ACFLAGS_SPEED_VALID | MODES_ACFLAGS_HEADING_VALID | MODES_ACFLAGS_NSEWSPD_VALID);
                mm->velocity = (int) sqrt((ns_vel * ns_vel) + (ew_vel * ew_vel));
                
                if (mm->velocity) {
                    mm->heading = (int) (atan2(ew_vel, ns_vel) * 180.0 / M_PI);
                    // We don't want negative values but a 0-360 scale
                    if (mm->heading < 0) mm->heading += 360;
                }
            }
            
        } else if (mesub == 3 || mesub == 4) {
            int airspeed = ((msg[7] & 0x7f) << 3) | (msg[8] >> 5);
            if (airspeed) {
                mm->bFlags |= MODES_ACFLAGS_SPEED_VALID;
                --airspeed;
                if (mesub == 4)  // If (supersonic) unit is 4 kts
                    {airspeed = airspeed << 2;}
                mm->velocity =  airspeed;
            }
            
            if (msg[5] & 0x04) {
                mm->bFlags |= MODES_ACFLAGS_HEADING_VALID;
                mm->heading = ((((msg[5] & 0x03) << 8) | msg[6]) * 45) >> 7;
            }
        }

        break;
    }
        
    case 5: case 6: case 7: case 8: {
        // Ground position
        int movement;

        if (check_imf && (msg[6] & 0x08))
            mm->addr |= MODES_NON_ICAO_ADDRESS;

        mm->bFlags |= MODES_ACFLAGS_AOG_VALID | MODES_ACFLAGS_AOG;
        mm->raw_latitude  = ((msg[6] & 3) << 15) | (msg[7] << 7) | (msg[8] >> 1);
        mm->raw_longitude = ((msg[8] & 1) << 16) | (msg[9] << 8) | (msg[10]);
        mm->bFlags       |= (mm->msg[6] & 0x04) ? MODES_ACFLAGS_LLODD_VALID 
            : MODES_ACFLAGS_LLEVEN_VALID;

        movement = ((msg[4] << 4) | (msg[5] >> 4)) & 0x007F;
        if ((movement) && (movement < 125)) {
            mm->bFlags |= MODES_ACFLAGS_SPEED_VALID;
            mm->velocity = decodeMovementField(movement);
        }

        if (msg[5] & 0x08) {
            mm->bFlags |= MODES_ACFLAGS_HEADING_VALID;
            mm->heading = ((((msg[5] << 4) | (msg[6] >> 4)) & 0x007F) * 45) >> 4;
        }

        mm->nuc_p = (14 - metype);
        break;
    }

    case 0: // Airborne position, baro altitude only
    case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: // Airborne position, baro
    case 20: case 21: case 22: { // Airborne position, GNSS HAE       
        int AC12Field = ((msg[5] << 4) | (msg[6] >> 4)) & 0x0FFF;

        if (check_imf && (msg[4] & 0x01))
            mm->addr |= MODES_NON_ICAO_ADDRESS;

        mm->bFlags |= MODES_ACFLAGS_AOG_VALID;

        if (metype != 0) {
            // Catch some common failure modes and don't mark them as valid
            // (so they won't be used for positioning)

            mm->raw_latitude  = ((msg[6] & 3) << 15) | (msg[7] << 7) | (msg[8] >> 1);
            mm->raw_longitude = ((msg[8] & 1) << 16) | (msg[9] << 8) | (msg[10]);

            if (AC12Field == 0 && mm->raw_longitude == 0 && (mm->raw_latitude & 0x0fff) == 0 && mm->metype == 15) {
                // Seen from at least:
                //   400F3F (Eurocopter ECC155 B1) - Bristow Helicopters
                //   4008F3 (BAE ATP) - Atlantic Airlines
                //   400648 (BAE ATP) - Atlantic Airlines
                // altitude == 0, longitude == 0, type == 15 and zeros in latitude LSB.
                // Can alternate with valid reports having type == 14
                Modes.stats_current.cpr_filtered++;
            } else {
                // Otherwise, assume it's valid.
                mm->bFlags       |= (mm->msg[6] & 0x04) ? MODES_ACFLAGS_LLODD_VALID 
                    : MODES_ACFLAGS_LLEVEN_VALID;
            }
        }

        if (AC12Field) {// Only attempt to decode if a valid (non zero) altitude is present
            mm->altitude = decodeAC12Field(AC12Field, &mm->unit);
            if (mm->altitude != INVALID_ALTITUDE)
                mm->bFlags  |= MODES_ACFLAGS_ALTITUDE_VALID;
        }

        if (metype == 0 || metype == 18 || metype == 22)
            mm->nuc_p = 0;
        else if (metype < 18)
            mm->nuc_p = (18 - metype);
        else
            mm->nuc_p = (29 - metype);
        
        break;
    }

    case 23: { // Test message
        if (mesub == 7) {               // (see 1090-WP-15-20)
            int ID13Field = (((msg[5] << 8) | msg[6]) & 0xFFF1)>>3;
            if (ID13Field) {
                mm->bFlags |= MODES_ACFLAGS_SQUAWK_VALID;
                mm->modeA   = decodeID13Field(ID13Field);
            }
        }
        break;
    }

    case 24: // Reserved for Surface System Status
        break;

    case 28: { // Extended Squitter Aircraft Status
        if (mesub == 1) {      // Emergency status squawk field
            int ID13Field = (((msg[5] << 8) | msg[6]) & 0x1FFF);
            if (ID13Field) {
                mm->bFlags |= MODES_ACFLAGS_SQUAWK_VALID;
                mm->modeA   = decodeID13Field(ID13Field);
            }

            if (check_imf && (msg[10] & 0x01))
                mm->addr |= MODES_NON_ICAO_ADDRESS;
        }
        break;
    }

    case 29: // Aircraft Trajectory Intent
        break;

    case 30: // Aircraft Operational Coordination
        break;

    case 31: // Aircraft Operational Status
        if (check_imf && (msg[10] & 0x01))
            mm->addr |= MODES_NON_ICAO_ADDRESS;
        break;

    default: 
        break;
    }
}

static void decodeCommB(struct modesMessage *mm)
{    
    unsigned char *msg = mm->msg;

    // This is a bit hairy as we don't know what the requested register was
    if (msg[4] == 0x20) { // BDS 2,0 Aircraft Identification
        decodeBDS20(mm);
    }
}

//
//=========================================================================
//
// These functions gets a decoded Mode S Message and prints it on the screen
// in a human readable format.
//
static void displayExtendedSquitter(struct modesMessage *mm) {
    printf("  Extended Squitter  Type: %d\n", mm->metype);
    printf("  Extended Squitter  Sub : %d\n", mm->mesub);
    printf("  Extended Squitter  Name: %s\n", getMEDescription(mm->metype, mm->mesub));

    // Decode the extended squitter message
    if (mm->metype >= 1 && mm->metype <= 4) { // Aircraft identification
        printf("    Aircraft Type  : %02X\n", mm->category);
        printf("    Identification : %s\n", (mm->bFlags & MODES_ACFLAGS_CALLSIGN_VALID) ? mm->flight : "invalid");
    } else if (mm->metype == 19) { // Airborne Velocity
        if (mm->mesub == 1 || mm->mesub == 2) {
            printf("    EW status         : %s\n", (mm->bFlags & MODES_ACFLAGS_EWSPEED_VALID)  ? "Valid" : "Unavailable");
            printf("    EW velocity       : %d\n", mm->ew_velocity);
            printf("    NS status         : %s\n", (mm->bFlags & MODES_ACFLAGS_NSSPEED_VALID)  ? "Valid" : "Unavailable");
            printf("    NS velocity       : %d\n", mm->ns_velocity);
            printf("    Vertical status   : %s\n", (mm->bFlags & MODES_ACFLAGS_VERTRATE_VALID) ? "Valid" : "Unavailable");
            printf("    Vertical rate src : %d\n", ((mm->msg[8] >> 4) & 1));
            printf("    Vertical rate     : %d\n", mm->vert_rate);
        } else if (mm->mesub == 3 || mm->mesub == 4) {
            printf("    Heading status    : %s\n", (mm->bFlags & MODES_ACFLAGS_HEADING_VALID)  ? "Valid" : "Unavailable");
            printf("    Heading           : %d\n", mm->heading);
            printf("    Airspeed status   : %s\n", (mm->bFlags & MODES_ACFLAGS_SPEED_VALID)    ? "Valid" : "Unavailable");
            printf("    Airspeed          : %d\n", mm->velocity);
            printf("    Vertical status   : %s\n", (mm->bFlags & MODES_ACFLAGS_VERTRATE_VALID) ? "Valid" : "Unavailable");
            printf("    Vertical rate src : %d\n", ((mm->msg[8] >> 4) & 1));
            printf("    Vertical rate     : %d\n", mm->vert_rate);
            
        } else {
            printf("    Unrecognized ME subtype: %d subtype: %d\n", mm->metype, mm->mesub);
        }
    } else if (mm->metype >= 5 && mm->metype <= 22) { // Airborne position Baro
        printf("    F flag   : %s\n", (mm->msg[6] & 0x04) ? "odd" : "even");
        printf("    T flag   : %s\n", (mm->msg[6] & 0x08) ? "UTC" : "non-UTC");
        if (mm->bFlags & MODES_ACFLAGS_ALTITUDE_VALID)
            printf("    Altitude : %d feet\n", mm->altitude);
        else
            printf("    Altitude : not valid\n");
        if (mm->bFlags & MODES_ACFLAGS_LATLON_VALID) {
            if (mm->bFlags & MODES_ACFLAGS_REL_CPR_USED)
                printf("    Local CPR decoding used.\n");
            else
                printf("    Global CPR decoding used.\n");
            printf("    Latitude : %f (%d)\n", mm->fLat, mm->raw_latitude);
            printf("    Longitude: %f (%d)\n", mm->fLon, mm->raw_longitude);
            printf("    NUCp:      %u\n", mm->nuc_p);
        } else {
            if (!(mm->bFlags & MODES_ACFLAGS_LLEITHER_VALID))
                printf("    Bad position data, not decoded.\n");
            printf("    Latitude : %d (not decoded)\n", mm->raw_latitude);
            printf("    Longitude: %d (not decoded)\n", mm->raw_longitude);
            printf("    NUCp:      %u\n", mm->nuc_p);
        }
    } else if (mm->metype == 28) { // Extended Squitter Aircraft Status
        if (mm->mesub == 1) {
            printf("    Emergency State: %s\n", es_str[(mm->msg[5] & 0xE0) >> 5]);
            printf("    Squawk: %04x\n", mm->modeA);
        } else {
            printf("    Unrecognized ME subtype: %d subtype: %d\n", mm->metype, mm->mesub);
        }
    } else if (mm->metype == 23) { // Test Message
        if (mm->mesub == 7) {
            printf("    Squawk: %04x\n", mm->modeA);
        } else {
            printf("    Unrecognized ME subtype: %d subtype: %d\n", mm->metype, mm->mesub);
        }
    } else {
        printf("    Unrecognized ME type: %d subtype: %d\n", mm->metype, mm->mesub);
    }
}

static void displayCommB(struct modesMessage *mm)
{
    if (mm->bds != 0)
        printf("  Comm-B BDS     : %02x (maybe)\n", mm->bds);

    // Decode the Comm-B message
    if (mm->msg[4] == 0x20 && (mm->bds == 0 || mm->bds == 0x20)) { // BDS 2,0 Aircraft identification
        printf("    BDS 2,0 Aircraft Identification : %s\n", mm->flight);
    } else {
        int i;
        printf("  Comm-B MB      : ");
        for (i = 4; i < 11; ++i)
            printf("%02x", mm->msg[i]);
        printf("\n");
    }        
}

void displayModesMessage(struct modesMessage *mm) {
    int j;

    // Handle only addresses mode first.
    if (Modes.onlyaddr) {
        printf("%06x\n", mm->addr);
        return;         // Enough for --onlyaddr mode
    }

    // Show the raw message.
    if (Modes.mlat && mm->timestampMsg) {
        printf("@%012" PRIX64, mm->timestampMsg);
    } else
        printf("*");

    for (j = 0; j < mm->msgbits/8; j++) printf("%02x", mm->msg[j]);
    printf(";\n");

    if (Modes.raw) {
        fflush(stdout); // Provide data to the reader ASAP
        return;         // Enough for --raw mode
    }

    if (mm->msgtype < 32)
        printf("CRC: %06x\n", mm->crc);

    if (mm->correctedbits != 0)
        printf("No. of bit errors fixed: %d\n", mm->correctedbits);

    if (mm->signalLevel > 0)
        printf("RSSI: %.1f dBFS\n", 10 * log10(mm->signalLevel));

    if (mm->score)
        printf("Score: %d\n", mm->score);

    if (mm->timestampMsg) {
        if (mm->timestampMsg == MAGIC_MLAT_TIMESTAMP)
            printf("This is a synthetic MLAT message.\n");
        else
            printf("Time: %.2fus (phase: %d)\n", mm->timestampMsg / 12.0, (unsigned int) (360 * (mm->timestampMsg % 6) / 6));
    }

    if (mm->msgtype == 0) { // DF 0
        printf("DF 0: Short Air-Air Surveillance.\n");
        printf("  VS             : %s\n",  (mm->msg[0] & 0x04) ? "Ground" : "Airborne");
        printf("  CC             : %d\n", ((mm->msg[0] & 0x02) >> 1));
        printf("  SL             : %d\n", ((mm->msg[1] & 0xE0) >> 5));
        printf("  Altitude       : %d %s\n", mm->altitude,
            (mm->unit == MODES_UNIT_METERS) ? "meters" : "feet");
        printf("  ICAO Address   : %06x\n", mm->addr);

    } else if (mm->msgtype == 4 || mm->msgtype == 20) {
        printf("DF %d: %s, Altitude Reply.\n", mm->msgtype,
            (mm->msgtype == 4) ? "Surveillance" : "Comm-B");
        printf("  Flight Status  : %s\n", fs_str[mm->fs]);
        printf("  DR             : %d\n", ((mm->msg[1] >> 3) & 0x1F));
        printf("  UM             : %d\n", (((mm->msg[1]  & 7) << 3) | (mm->msg[2] >> 5)));
        printf("  Altitude       : %d %s\n", mm->altitude,
            (mm->unit == MODES_UNIT_METERS) ? "meters" : "feet");
        printf("  ICAO Address   : %06x\n", mm->addr);

        if (mm->msgtype == 20) {
            displayCommB(mm);
        }
    } else if (mm->msgtype == 5 || mm->msgtype == 21) {
        printf("DF %d: %s, Identity Reply.\n", mm->msgtype,
            (mm->msgtype == 5) ? "Surveillance" : "Comm-B");
        printf("  Flight Status  : %s\n", fs_str[mm->fs]);
        printf("  DR             : %d\n", ((mm->msg[1] >> 3) & 0x1F));
        printf("  UM             : %d\n", (((mm->msg[1]  & 7) << 3) | (mm->msg[2] >> 5)));
        printf("  Squawk         : %04x\n", mm->modeA);
        printf("  ICAO Address   : %06x\n", mm->addr);

        if (mm->msgtype == 21) {
            displayCommB(mm);
        }
    } else if (mm->msgtype == 11) { // DF 11
        printf("DF 11: All Call Reply.\n");
        printf("  Capability  : %d (%s)\n", mm->ca, ca_str[mm->ca]);
        printf("  ICAO Address: %06x\n", mm->addr);
        if (mm->iid > 16)
            {printf("  IID         : SI-%02d\n", mm->iid-16);}
        else
            {printf("  IID         : II-%02d\n", mm->iid);}

    } else if (mm->msgtype == 16) { // DF 16
        printf("DF 16: Long Air to Air ACAS\n");
        printf("  VS             : %s\n",  (mm->msg[0] & 0x04) ? "Ground" : "Airborne");
        printf("  CC             : %d\n", ((mm->msg[0] & 0x02) >> 1));
        printf("  SL             : %d\n", ((mm->msg[1] & 0xE0) >> 5));
        printf("  Altitude       : %d %s\n", mm->altitude,
            (mm->unit == MODES_UNIT_METERS) ? "meters" : "feet");
        printf("  ICAO Address   : %06x\n", mm->addr);

    } else if (mm->msgtype == 17) { // DF 17
        printf("DF 17: ADS-B message.\n");
        printf("  Capability     : %d (%s)\n", mm->ca, ca_str[mm->ca]);
        printf("  ICAO Address   : %06x\n", mm->addr);
        displayExtendedSquitter(mm);
    } else if (mm->msgtype == 18) { // DF 18 
        printf("DF 18: Extended Squitter.\n");
        printf("  Control Field : %d (%s)\n", mm->cf, cf_str[mm->cf]);
        if ((mm->cf == 0) || (mm->cf == 1) || (mm->cf == 5) || (mm->cf == 6)) {
            if (mm->cf == 1 || mm->cf == 5) {
                printf("  Other Address : %06x\n", mm->addr);
            } else {
                printf("  ICAO Address  : %06x\n", mm->addr);
            }
            displayExtendedSquitter(mm);
        }             

    } else if (mm->msgtype == 19) { // DF 19
        printf("DF 19: Military Extended Squitter.\n");

    } else if (mm->msgtype == 22) { // DF 22
        printf("DF 22: Military Use.\n");

    } else if (mm->msgtype == 24) { // DF 24
        printf("DF 24: Comm D Extended Length Message.\n");

    } else if (mm->msgtype == 32) { // DF 32 is special code we use for Mode A/C
        printf("SSR : Mode A/C Reply.\n");
        if (mm->fs & 0x0080) {
            printf("  Mode A : %04x IDENT\n", mm->modeA);
        } else {
            printf("  Mode A : %04x\n", mm->modeA);
            if (mm->bFlags & MODES_ACFLAGS_ALTITUDE_VALID)
                {printf("  Mode C : %d feet\n", mm->altitude);}
        }

    } else {
        printf("DF %d: Unknown DF Format.\n", mm->msgtype);
    }

    printf("\n");
}

//
//=========================================================================
//
// When a new message is available, because it was decoded from the RTL device, 
// file, or received in the TCP input port, or any other way we can receive a 
// decoded message, we call this function in order to use the message.
//
// Basically this function passes a raw message to the upper layers for further
// processing and visualization
//
void useModesMessage(struct modesMessage *mm) {
    struct aircraft *a;

    ++Modes.stats_current.messages_total;

    // Track aircraft state
    a = trackUpdateFromMessage(mm);

    // In non-interactive non-quiet mode, display messages on standard output
    if (!Modes.interactive && !Modes.quiet && (!Modes.show_only || mm->addr == Modes.show_only)) {
        displayModesMessage(mm);
    }

    // Feed output clients.
    // If in --net-verbatim mode, do this for all messages.
    // Otherwise, apply a sanity-check filter and only
    // forward messages when we have seen two of them.

    if (Modes.net) {
        if (Modes.net_verbatim || mm->msgtype == 32) {
            // Unconditionally send
            modesQueueOutput(mm);
        } else if (a->messages > 1) {
            // If this is the second message, and we
            // squelched the first message, then re-emit the
            // first message now.
            if (!Modes.net_verbatim && a->messages == 2) {
                modesQueueOutput(&a->first_message);
            }
            modesQueueOutput(mm);
        }
    }
}

//
// ===================== Mode S detection and decoding  ===================
//
