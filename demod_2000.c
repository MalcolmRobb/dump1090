// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// demod_2000.c: 2MHz Mode S demodulator.
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

// Mode S 2.0MHz demodulator

//
// ===================== Mode A/C detection and decoding  ===================
//
//
// This table is used to build the Mode A/C variable called ModeABits.Each 
// bit period is inspected, and if it's value exceeds the threshold limit, 
// then the value in this table is or-ed into ModeABits.
//
// At the end of message processing, ModeABits will be the decoded ModeA value.
//
// We can also flag noise in bits that should be zeros - the xx bits. Noise in
// these bits cause bits (31-16) in ModeABits to be set. Then at the end of message
// processing we can test for errors by looking at these bits.
//
uint32_t ModeABitTable[24] = {
0x00000000, // F1 = 1
0x00000010, // C1
0x00001000, // A1
0x00000020, // C2 
0x00002000, // A2
0x00000040, // C4
0x00004000, // A4
0x40000000, // xx = 0  Set bit 30 if we see this high
0x00000100, // B1 
0x00000001, // D1
0x00000200, // B2
0x00000002, // D2
0x00000400, // B4
0x00000004, // D4
0x00000000, // F2 = 1
0x08000000, // xx = 0  Set bit 27 if we see this high
0x04000000, // xx = 0  Set bit 26 if we see this high
0x00000080, // SPI
0x02000000, // xx = 0  Set bit 25 if we see this high
0x01000000, // xx = 0  Set bit 24 if we see this high
0x00800000, // xx = 0  Set bit 23 if we see this high
0x00400000, // xx = 0  Set bit 22 if we see this high
0x00200000, // xx = 0  Set bit 21 if we see this high
0x00100000, // xx = 0  Set bit 20 if we see this high
};
//
// This table is used to produce an error variable called ModeAErrs.Each 
// inter-bit period is inspected, and if it's value falls outside of the 
// expected range, then the value in this table is or-ed into ModeAErrs.
//
// At the end of message processing, ModeAErrs will indicate if we saw 
// any inter-bit anomolies, and the bits that are set will show which 
// bits had them.
//
uint32_t ModeAMidTable[24] = {
0x80000000, // F1 = 1  Set bit 31 if we see F1_C1  error
0x00000010, // C1      Set bit  4 if we see C1_A1  error
0x00001000, // A1      Set bit 12 if we see A1_C2  error
0x00000020, // C2      Set bit  5 if we see C2_A2  error
0x00002000, // A2      Set bit 13 if we see A2_C4  error
0x00000040, // C4      Set bit  6 if we see C3_A4  error
0x00004000, // A4      Set bit 14 if we see A4_xx  error
0x40000000, // xx = 0  Set bit 30 if we see xx_B1  error
0x00000100, // B1      Set bit  8 if we see B1_D1  error
0x00000001, // D1      Set bit  0 if we see D1_B2  error
0x00000200, // B2      Set bit  9 if we see B2_D2  error
0x00000002, // D2      Set bit  1 if we see D2_B4  error
0x00000400, // B4      Set bit 10 if we see B4_D4  error
0x00000004, // D4      Set bit  2 if we see D4_F2  error
0x20000000, // F2 = 1  Set bit 29 if we see F2_xx  error
0x08000000, // xx = 0  Set bit 27 if we see xx_xx  error
0x04000000, // xx = 0  Set bit 26 if we see xx_SPI error
0x00000080, // SPI     Set bit 15 if we see SPI_xx error
0x02000000, // xx = 0  Set bit 25 if we see xx_xx  error
0x01000000, // xx = 0  Set bit 24 if we see xx_xx  error
0x00800000, // xx = 0  Set bit 23 if we see xx_xx  error
0x00400000, // xx = 0  Set bit 22 if we see xx_xx  error
0x00200000, // xx = 0  Set bit 21 if we see xx_xx  error
0x00100000, // xx = 0  Set bit 20 if we see xx_xx  error
};
//
// The "off air" format is,,
// _F1_C1_A1_C2_A2_C4_A4_xx_B1_D1_B2_D2_B4_D4_F2_xx_xx_SPI_
//
// Bit spacing is 1.45uS, with 0.45uS high, and 1.00us low. This is a problem
// because we ase sampling at 2Mhz (500nS) so we are below Nyquist. 
//
// The bit spacings are..
// F1 :  0.00,   
//       1.45,  2.90,  4.35,  5.80,  7.25,  8.70, 
// X  : 10.15, 
//    : 11.60, 13.05, 14.50, 15.95, 17.40, 18.85, 
// F2 : 20.30, 
// X  : 21.75, 23.20, 24.65 
//
// This equates to the following sample point centers at 2Mhz.
// [ 0.0], 
// [ 2.9], [ 5.8], [ 8.7], [11.6], [14.5], [17.4], 
// [20.3], 
// [23.2], [26.1], [29.0], [31.9], [34.8], [37.7]
// [40.6]
// [43.5], [46.4], [49.3]
//
// We know that this is a supposed to be a binary stream, so the signal
// should either be a 1 or a 0. Therefore, any energy above the noise level 
// in two adjacent samples must be from the same pulse, so we can simply 
// add the values together.. 
// 
int detectModeA(uint16_t *m, struct modesMessage *mm)
  {
  int j, lastBitWasOne;
  int ModeABits = 0;
  int ModeAErrs = 0;
  int byte, bit;
  int thisSample, lastBit, lastSpace = 0; 
  int m0, m1, m2, m3, mPhase;
  int n0, n1, n2 ,n3;
  int F1_sig, F1_noise;
  int F2_sig, F2_noise;
  int fSig, fNoise, fLevel, fLoLo;

  // m[0] contains the energy from    0 ->  499 nS
  // m[1] contains the energy from  500 ->  999 nS
  // m[2] contains the energy from 1000 -> 1499 nS
  // m[3] contains the energy from 1500 -> 1999 nS
  //
  // We are looking for a Frame bit (F1) whose width is 450nS, followed by
  // 1000nS of quiet.
  //
  // The width of the frame bit is 450nS, which is 90% of our sample rate.
  // Therefore, in an ideal world, all the energy for the frame bit will be
  // in a single sample, preceeded by (at least) one zero, and followed by 
  // two zeros, Best case we can look for ...
  //
  // 0 - 1 - 0 - 0
  //
  // However, our samples are not phase aligned, so some of the energy from 
  // each bit could be spread over two consecutive samples. Worst case is
  // that we sample half in one bit, and half in the next. In that case, 
  // we're looking for 
  //
  // 0 - 0.5 - 0.5 - 0.

  m0 = m[0]; m1 = m[1];

  if (m0 >= m1)   // m1 *must* be bigger than m0 for this to be F1
    {return (0);}

  m2 = m[2]; m3 = m[3];

  // 
  // if (m2 <= m0), then assume the sample bob on (Phase == 0), so don't look at m3 
  if ((m2 <= m0) || (m2 < m3))
    {m3 = m2; m2 = m0;}

  if (  (m3 >= m1)   // m1 must be bigger than m3
     || (m0 >  m2)   // m2 can be equal to m0 if ( 0,1,0,0 )
     || (m3 >  m2) ) // m2 can be equal to m3 if ( 0,1,0,0 )
    {return (0);}

  // m0 = noise
  // m1 = noise + (signal *    X))
  // m2 = noise + (signal * (1-X))
  // m3 = noise
  //
  // Hence, assuming all 4 samples have similar amounts of noise in them 
  //      signal = (m1 + m2) - ((m0 + m3) * 2)
  //      noise  = (m0 + m3) / 2
  //
  F1_sig   = (m1 + m2) - ((m0 + m3) << 1);
  F1_noise = (m0 + m3) >> 1;

  if ( (F1_sig < MODEAC_MSG_SQUELCH_LEVEL) // minimum required  F1 signal amplitude
    || (F1_sig < (F1_noise << 2)) )        // minimum allowable Sig/Noise ratio 4:1
    {return (0);}

  // If we get here then we have a potential F1, so look for an equally valid F2 20.3uS later
  //
  // Our F1 is centered somewhere between samples m[1] and m[2]. We can guestimate where F2 is 
  // by comparing the ratio of m1 and m2, and adding on 20.3 uS (40.6 samples)
  //
  mPhase = ((m2 * 20) / (m1 + m2));
  byte   = (mPhase + 812) / 20; 
  n0     = m[byte++]; n1 = m[byte++]; 

  if (n0 >= n1)   // n1 *must* be bigger than n0 for this to be F2
    {return (0);}

  n2 = m[byte++];
  // 
  // if the sample bob on (Phase == 0), don't look at n3 
  //
  if ((mPhase + 812) % 20)
    {n3 = m[byte++];}
  else
    {n3 = n2; n2 = n0;}

  if (  (n3 >= n1)   // n1 must be bigger than n3
     || (n0 >  n2)   // n2 can be equal to n0 ( 0,1,0,0 )
     || (n3 >  n2) ) // n2 can be equal to n3 ( 0,1,0,0 )
    {return (0);}

  F2_sig   = (n1 + n2) - ((n0 + n3) << 1);
  F2_noise = (n0 + n3) >> 1;

  if ( (F2_sig < MODEAC_MSG_SQUELCH_LEVEL) // minimum required  F2 signal amplitude
    || (F2_sig < (F2_noise << 2)) )       // maximum allowable Sig/Noise ratio 4:1
    {return (0);}

  fSig          = (F1_sig   + F2_sig)   >> 1;
  fNoise        = (F1_noise + F2_noise) >> 1;
  fLoLo         = fNoise    + (fSig >> 2);       // 1/2
  fLevel        = fNoise    + (fSig >> 1);
  lastBitWasOne = 1;
  lastBit       = F1_sig;
  //
  // Now step by a half ModeA bit, 0.725nS, which is 1.45 samples, which is 29/20
  // No need to do bit 0 because we've already selected it as a valid F1
  // Do several bits past the SPI to increase error rejection
  //
  for (j = 1, mPhase += 29; j < 48; mPhase += 29, j ++)
    {
    byte  = 1 + (mPhase / 20);
    
    thisSample = m[byte] - fNoise;
    if (mPhase % 20)                     // If the bit is split over two samples...
      {thisSample += (m[byte+1] - fNoise);}  //    add in the second sample's energy

     // If we're calculating a space value
    if (j & 1)               
      {lastSpace = thisSample;}

    else 
      {// We're calculating a new bit value
      bit = j >> 1;
      if (thisSample >= fLevel)
        {// We're calculating a new bit value, and its a one
        ModeABits |= ModeABitTable[bit--];  // or in the correct bit

        if (lastBitWasOne)
          { // This bit is one, last bit was one, so check the last space is somewhere less than one
          if ( (lastSpace >= (thisSample>>1)) || (lastSpace >= lastBit) )
            {ModeAErrs |= ModeAMidTable[bit];}
          }

        else              
          {// This bit,is one, last bit was zero, so check the last space is somewhere less than one
          if (lastSpace >= (thisSample >> 1))
            {ModeAErrs |= ModeAMidTable[bit];}
          }

        lastBitWasOne = 1;
        }

      
      else 
        {// We're calculating a new bit value, and its a zero
        if (lastBitWasOne)
          { // This bit is zero, last bit was one, so check the last space is somewhere in between
          if (lastSpace >= lastBit)
            {ModeAErrs |= ModeAMidTable[bit];}
          }

        else              
          {// This bit,is zero, last bit was zero, so check the last space is zero too
          if (lastSpace >= fLoLo)
            {ModeAErrs |= ModeAMidTable[bit];}
          }

        lastBitWasOne = 0;   
        }

      lastBit = (thisSample >> 1); 
      }
    }

  //
  // Output format is : 00:A4:A2:A1:00:B4:B2:B1:00:C4:C2:C1:00:D4:D2:D1
  //
  if ((ModeABits < 3) || (ModeABits & 0xFFFF8808) || (ModeAErrs) )
    {return (ModeABits = 0);}

  mm->signalLevel = (fSig + fNoise) * (fSig + fNoise) / MAX_POWER;

  return ModeABits;
  }

// ============================== Debugging =================================
//
// Helper function for dumpMagnitudeVector().
// It prints a single bar used to display raw signals.
//
// Since every magnitude sample is between 0-255, the function uses
// up to 63 characters for every bar. Every character represents
// a length of 4, 3, 2, 1, specifically:
//
// "O" is 4
// "o" is 3
// "-" is 2
// "." is 1
//
static void dumpMagnitudeBar(int index, int magnitude) {
    char *set = " .-o";
    char buf[256];
    int div = magnitude / 256 / 4;
    int rem = magnitude / 256 % 4;

    memset(buf,'O',div);
    buf[div] = set[rem];
    buf[div+1] = '\0';

    if (index >= 0)
        printf("[%.3d] |%-66s 0x%04X\n", index, buf, magnitude);
    else
        printf("[%.2d] |%-66s 0x%04X\n", index, buf, magnitude);
}
//
//=========================================================================
//
// Display an ASCII-art alike graphical representation of the undecoded
// message as a magnitude signal.
//
// The message starts at the specified offset in the "m" buffer.
// The function will display enough data to cover a short 56 bit message.
//
// If possible a few samples before the start of the messsage are included
// for context.
//
static void dumpMagnitudeVector(uint16_t *m, uint32_t offset) {
    uint32_t padding = 5; // Show a few samples before the actual start.
    uint32_t start = (offset < padding) ? 0 : offset-padding;
    uint32_t end = offset + (MODES_PREAMBLE_SAMPLES)+(MODES_SHORT_MSG_SAMPLES) - 1;
    uint32_t j;

    for (j = start; j <= end; j++) {
        dumpMagnitudeBar(j-offset, m[j]);
    }
}
//
//=========================================================================
//
// Produce a raw representation of the message as a Javascript file
// loadable by debug.html.
//
static void dumpRawMessageJS(char *descr, unsigned char *msg,
                             uint16_t *m, uint32_t offset, struct errorinfo *ei)
{
    int padding = 5; // Show a few samples before the actual start.
    int start = offset - padding;
    int end = offset + (MODES_PREAMBLE_SAMPLES)+(MODES_LONG_MSG_SAMPLES) - 1;
    FILE *fp;
    int j;

    if ((fp = fopen("frames.js","a")) == NULL) {
        fprintf(stderr, "Error opening frames.js: %s\n", strerror(errno));
        exit(1);
    }

    fprintf(fp,"frames.push({\"descr\": \"%s\", \"mag\": [", descr);
    for (j = start; j <= end; j++) {
        fprintf(fp,"%d", j < 0 ? 0 : m[j]);
        if (j != end) fprintf(fp,",");
    }
    fprintf(fp, "], ");
    for (j = 0; j < MODES_MAX_BITERRORS; ++j)
        fprintf(fp,"\"fix%d\": %d, ", j, ei->bit[j]);
    fprintf(fp, "\"bits\": %d, \"hex\": \"", modesMessageLenByType(msg[0]>>3));
    for (j = 0; j < MODES_LONG_MSG_BYTES; j++)
        fprintf(fp,"\\x%02x",msg[j]);
    fprintf(fp,"\"});\n");
    fclose(fp);
}
//
//=========================================================================
//
// This is a wrapper for dumpMagnitudeVector() that also show the message
// in hex format with an additional description.
//
// descr  is the additional message to show to describe the dump.
// msg    points to the decoded message
// m      is the original magnitude vector
// offset is the offset where the message starts
//
// The function also produces the Javascript file used by debug.html to
// display packets in a graphical format if the Javascript output was
// enabled.
//
static void dumpRawMessage(char *descr, unsigned char *msg, uint16_t *m, uint32_t offset) {
    int  j;
    int  msgtype = msg[0] >> 3;
    struct errorinfo *ei = NULL;

    if (msgtype == 17) {
        int len = modesMessageLenByType(msgtype);
        uint32_t csum = modesChecksum(msg, len);
        ei = modesChecksumDiagnose(csum, len);
    }

    if (Modes.debug & MODES_DEBUG_JS) {
        dumpRawMessageJS(descr, msg, m, offset, ei);
        return;
    }

    printf("\n--- %s\n    ", descr);
    for (j = 0; j < MODES_LONG_MSG_BYTES; j++) {
        printf("%02x",msg[j]);
        if (j == MODES_SHORT_MSG_BYTES-1) printf(" ... ");
    }
    printf(" (DF %d, Fixable: %d)\n", msgtype, ei ? ei->errors : 0);
    dumpMagnitudeVector(m,offset);
    printf("---\n\n");
}

//
//=========================================================================
//
// Return -1 if the message is out of fase left-side
// Return  1 if the message is out of fase right-size
// Return  0 if the message is not particularly out of phase.
//
// Note: this function will access pPreamble[-1], so the caller should make sure to
// call it only if we are not at the start of the current buffer
//
static int detectOutOfPhase(uint16_t *pPreamble) {
    if (pPreamble[ 3] > pPreamble[2]/3) return  1;
    if (pPreamble[10] > pPreamble[9]/3) return  1;
    if (pPreamble[ 6] > pPreamble[7]/3) return -1;
    if (pPreamble[-1] > pPreamble[1]/3) return -1;
    return 0;
}


static uint16_t clamped_scale(uint16_t v, uint16_t scale) {
    uint32_t scaled = (uint32_t)v * scale / 16384;
    if (scaled > 65535) return 65535;
    return (uint16_t) scaled;
}
// This function decides whether we are sampling early or late,
// and by approximately how much, by looking at the energy in
// preamble bits before and after the expected pulse locations.
//
// It then deals with one sample pair at a time, comparing samples
// to make a decision about the bit value. Based on this decision it
// modifies the sample value of the *adjacent* sample which will
// contain some of the energy from the bit we just inspected.
//
// pPayload[0] should be the start of the preamble,
// pPayload[-1 .. MODES_PREAMBLE_SAMPLES + MODES_LONG_MSG_SAMPLES - 1] should be accessible.
// pPayload[MODES_PREAMBLE_SAMPLES .. MODES_PREAMBLE_SAMPLES + MODES_LONG_MSG_SAMPLES - 1] will be updated.
static void applyPhaseCorrection(uint16_t *pPayload) {
    int j;

    // we expect 1 bits at 0, 2, 7, 9
    // and 0 bits at -1, 1, 3, 4, 5, 6, 8, 10, 11, 12, 13, 14
    // use bits -1,6 for early detection (bit 0/7 arrived a little early, our sample period starts after the bit phase so we include some of the next bit)
    // use bits 3,10 for late detection (bit 2/9 arrived a little late, our sample period starts before the bit phase so we include some of the last bit)

    uint32_t onTime = (pPayload[0] + pPayload[2] + pPayload[7] + pPayload[9]);
    uint32_t early = (pPayload[-1] + pPayload[6]) << 1;
    uint32_t late = (pPayload[3] + pPayload[10]) << 1;

    if (onTime == 0 && early == 0 && late == 0) {
        // Blah, can't do anything with this, avoid a divide-by-zero
        return;
    }

    if (early > late) {
        // Our sample period starts late and so includes some of the next bit.

        uint16_t scaleUp = 16384 + 16384 * early / (early + onTime);   // 1 + early / (early+onTime)
        uint16_t scaleDown = 16384 - 16384 * early / (early + onTime); // 1 - early / (early+onTime)

        // trailing bits are 0; final data sample will be a bit low.
        pPayload[MODES_PREAMBLE_SAMPLES + MODES_LONG_MSG_SAMPLES - 1] =
            clamped_scale(pPayload[MODES_PREAMBLE_SAMPLES + MODES_LONG_MSG_SAMPLES - 1],  scaleUp);
        for (j = MODES_PREAMBLE_SAMPLES + MODES_LONG_MSG_SAMPLES - 2; j > MODES_PREAMBLE_SAMPLES; j -= 2) {
            if (pPayload[j] > pPayload[j+1]) {
                // x [1 0] y
                // x overlapped with the "1" bit and is slightly high
                pPayload[j-1] = clamped_scale(pPayload[j-1], scaleDown);
            } else {
                // x [0 1] y
                // x overlapped with the "0" bit and is slightly low
                pPayload[j-1] = clamped_scale(pPayload[j-1], scaleUp);
            }
        }
    } else {
        // Our sample period starts early and so includes some of the previous bit.

        uint16_t scaleUp = 16384 + 16384 * late / (late + onTime);   // 1 + late / (late+onTime)
        uint16_t scaleDown = 16384 - 16384 * late / (late + onTime); // 1 - late / (late+onTime)

        // leading bits are 0; first data sample will be a bit low.
        pPayload[MODES_PREAMBLE_SAMPLES] = clamped_scale(pPayload[MODES_PREAMBLE_SAMPLES], scaleUp);
        for (j = MODES_PREAMBLE_SAMPLES; j < MODES_PREAMBLE_SAMPLES + MODES_LONG_MSG_SAMPLES - 2; j += 2) {
            if (pPayload[j] > pPayload[j+1]) {
                // x [1 0] y
                // y overlapped with the "0" bit and is slightly low
                pPayload[j+2] = clamped_scale(pPayload[j+2], scaleUp);
            } else {
                // x [0 1] y
                // y overlapped with the "1" bit and is slightly high
                pPayload[j+2] = clamped_scale(pPayload[j+2], scaleDown);
            }
        }
    }
}
//
//=========================================================================
//
// Detect a Mode S messages inside the magnitude buffer pointed by 'm' and of
// size 'mlen' bytes. Every detected Mode S message is convert it into a
// stream of bits and passed to the function to display it.
//
void demodulate2000(struct mag_buf *mag) {
    struct modesMessage mm;
    unsigned char msg[MODES_LONG_MSG_BYTES], *pMsg;
    uint16_t aux[MODES_PREAMBLE_SAMPLES+MODES_LONG_MSG_SAMPLES+1];
    uint32_t j;
    int use_correction = 0;
    unsigned mlen = mag->length;
    uint16_t *m = mag->data;

    memset(&mm, 0, sizeof(mm));

    // The Mode S preamble is made of impulses of 0.5 microseconds at
    // the following time offsets:
    //
    // 0   - 0.5 usec: first impulse.
    // 1.0 - 1.5 usec: second impulse.
    // 3.5 - 4   usec: third impulse.
    // 4.5 - 5   usec: last impulse.
    // 
    // Since we are sampling at 2 Mhz every sample in our magnitude vector
    // is 0.5 usec, so the preamble will look like this, assuming there is
    // an impulse at offset 0 in the array:
    //
    // 0   -----------------
    // 1   -
    // 2   ------------------
    // 3   --
    // 4   -
    // 5   --
    // 6   -
    // 7   ------------------
    // 8   --
    // 9   -------------------
    //
    for (j = 0; j < mlen; j++) {
        int high, i, errors, errors56, errorsTy; 
        uint16_t *pPreamble, *pPayload, *pPtr;
        uint8_t  theByte, theErrs;
        int msglen, scanlen;
        uint32_t sigLevel, noiseLevel;
        uint16_t snr;
        int message_ok;

        pPreamble = &m[j];
        pPayload  = &m[j+MODES_PREAMBLE_SAMPLES];

        // Rather than clear the whole mm structure, just clear the parts which are required. The clear
        // is required for every bit of the input stream, and we don't want to be memset-ing the whole
        // modesMessage structure two million times per second if we don't have to..
        mm.bFlags          =
        mm.correctedbits   = 0;

        if (!use_correction)  // This is not a re-try with phase correction
            {                 // so try to find a new preamble

            if (Modes.mode_ac) 
                {
                int ModeA = detectModeA(pPreamble, &mm);

                if (ModeA) // We have found a valid ModeA/C in the data                    
                    {
                    mm.timestampMsg = mag->sampleTimestamp + ((j+1) * 6);

                    // compute message receive time as block-start-time + difference in the 12MHz clock
                    mm.sysTimestampMsg = mag->sysTimestamp; // start of block time
                    mm.sysTimestampMsg.tv_nsec += receiveclock_ns_elapsed(mag->sampleTimestamp, mm.timestampMsg);
                    normalize_timespec(&mm.sysTimestampMsg);

                    // Decode the received message
                    decodeModeAMessage(&mm, ModeA);

                    // Pass data to the next layer
                    useModesMessage(&mm);

                    j += MODEAC_MSG_SAMPLES;
                    Modes.stats_current.demod_modeac++;
                    continue;
                    }
                }

            // First check of relations between the first 10 samples
            // representing a valid preamble. We don't even investigate further
            // if this simple test is not passed
            if (!(pPreamble[0] > pPreamble[1] &&
                  pPreamble[1] < pPreamble[2] &&
                  pPreamble[2] > pPreamble[3] &&
                  pPreamble[3] < pPreamble[0] &&
                  pPreamble[4] < pPreamble[0] &&
                  pPreamble[5] < pPreamble[0] &&
                  pPreamble[6] < pPreamble[0] &&
                  pPreamble[7] > pPreamble[8] &&
                  pPreamble[8] < pPreamble[9] &&
                  pPreamble[9] > pPreamble[6]))
            {
                if (Modes.debug & MODES_DEBUG_NOPREAMBLE &&
                    *pPreamble  > MODES_DEBUG_NOPREAMBLE_LEVEL)
                    dumpRawMessage("Unexpected ratio among first 10 samples", msg, m, j);
                continue;
            }

            // The samples between the two spikes must be < than the average
            // of the high spikes level. We don't test bits too near to
            // the high levels as signals can be out of phase so part of the
            // energy can be in the near samples
            high = (pPreamble[0] + pPreamble[2] + pPreamble[7] + pPreamble[9]) / 6;
            if (pPreamble[4] >= high ||
                pPreamble[5] >= high)
            {
                if (Modes.debug & MODES_DEBUG_NOPREAMBLE &&
                    *pPreamble  > MODES_DEBUG_NOPREAMBLE_LEVEL)
                    dumpRawMessage("Too high level in samples between 3 and 6", msg, m, j);
                continue;
            }

            // Similarly samples in the range 11-14 must be low, as it is the
            // space between the preamble and real data. Again we don't test
            // bits too near to high levels, see above
            if (pPreamble[11] >= high ||
                pPreamble[12] >= high ||
                pPreamble[13] >= high ||
                pPreamble[14] >= high)
            {
                if (Modes.debug & MODES_DEBUG_NOPREAMBLE &&
                    *pPreamble  > MODES_DEBUG_NOPREAMBLE_LEVEL)
                    dumpRawMessage("Too high level in samples between 10 and 15", msg, m, j);
                continue;
            }
            Modes.stats_current.demod_preambles++;
        } 

        else {
            // If the previous attempt with this message failed, retry using
            // magnitude correction
            // Make a copy of the Payload, and phase correct the copy
            memcpy(aux, &pPreamble[-1], sizeof(aux));
            applyPhaseCorrection(&aux[1]);
            pPayload = &aux[1 + MODES_PREAMBLE_SAMPLES];
            // TODO ... apply other kind of corrections
            }

        // Decode all the next 112 bits, regardless of the actual message
        // size. We'll check the actual message type later
        pMsg    = &msg[0];
        pPtr    = pPayload;
        theByte = 0;
        theErrs = 0; errorsTy = 0;
        errors  = 0; errors56 = 0;

        // We should have 4 'bits' of 0/1 and 1/0 samples in the preamble, 
        // so include these in the signal strength 
        sigLevel = pPreamble[0] + pPreamble[2] + pPreamble[7] + pPreamble[9];
        noiseLevel = pPreamble[1] + pPreamble[3] + pPreamble[4] + pPreamble[6] + pPreamble[8];

        msglen = scanlen = MODES_LONG_MSG_BITS;
        for (i = 0; i < scanlen; i++) {
            uint32_t a = *pPtr++;
            uint32_t b = *pPtr++;

            if      (a > b) 
                {theByte |= 1; if (i < 56) { sigLevel += a; noiseLevel += b; }}
            else if (a < b) 
                {/*theByte |= 0;*/ if (i < 56) { sigLevel += b; noiseLevel += a; }}
            else {
                if (i < 56) { sigLevel += a; noiseLevel += a; }
                if (i >= MODES_SHORT_MSG_BITS) //(a == b), and we're in the long part of a frame
                    {errors++;  /*theByte |= 0;*/}
                else if (i >= 5)                    //(a == b), and we're in the short part of a frame
                    {scanlen = MODES_LONG_MSG_BITS; errors56 = ++errors;/*theByte |= 0;*/}
                else if (i)                         //(a == b), and we're in the message type part of a frame
                    {errorsTy = errors56 = ++errors; theErrs |= 1; /*theByte |= 0;*/}
                else                                //(a == b), and we're in the first bit of the message type part of a frame
                    {errorsTy = errors56 = ++errors; theErrs |= 1; theByte |= 1;}
            }

            if ((i & 7) == 7) 
              {*pMsg++ = theByte;}
            else if (i == 4) {
              msglen  = modesMessageLenByType(theByte);
              if (errors == 0)
                  {scanlen = msglen;}
            }

            theByte = theByte << 1;
            if (i < 7)
              {theErrs = theErrs << 1;}

            // If we've exceeded the permissible number of encoding errors, abandon ship now
            if (errors > MODES_MSG_ENCODER_ERRS) {

                if        (i < MODES_SHORT_MSG_BITS) {
                    msglen = 0;

                } else if ((errorsTy == 1) && (theErrs == 0x80)) {
                    // If we only saw one error in the first bit of the byte of the frame, then it's possible 
                    // we guessed wrongly about the value of the bit. We may be able to correct it by guessing
                    // the other way.
                    //
                    // We guessed a '1' at bit 7, which is the DF length bit == 112 Bits.
                    // Inverting bit 7 will change the message type from a long to a short. 
                    // Invert the bit, cross your fingers and carry on.
                    msglen  = MODES_SHORT_MSG_BITS;
                    msg[0] ^= theErrs; errorsTy = 0;
                    errors  = errors56; // revert to the number of errors prior to bit 56

                } else if (i < MODES_LONG_MSG_BITS) {
                    msglen = MODES_SHORT_MSG_BITS;
                    errors = errors56;

                } else {
                    msglen = MODES_LONG_MSG_BITS;
                }

            break;
            }
        }

        // Ensure msglen is consistent with the DF type
        if (msglen > 0) {
            i = modesMessageLenByType(msg[0] >> 3);
            if      (msglen > i) {msglen = i;}
            else if (msglen < i) {msglen = 0;}
        }

        //
        // If we guessed at any of the bits in the DF type field, then look to see if our guess was sensible.
        // Do this by looking to see if the original guess results in the DF type being one of the ICAO defined
        // message types. If it isn't then toggle the guessed bit and see if this new value is ICAO defined.
        // if the new value is ICAO defined, then update it in our message.
        if ((msglen) && (errorsTy == 1) && (theErrs & 0x78)) {
            // We guessed at one (and only one) of the message type bits. See if our guess is "likely" 
            // to be correct by comparing the DF against a list of known good DF's
            int      thisDF      = ((theByte = msg[0]) >> 3) & 0x1f;
            uint32_t validDFbits = 0x017F0831;   // One bit per 32 possible DF's. Set bits 0,4,5,11,16.17.18.19,20,21,22,24
            uint32_t thisDFbit   = (1 << thisDF);
            if (0 == (validDFbits & thisDFbit)) {
                // The current DF is not ICAO defined, so is probably an errors. 
                // Toggle the bit we guessed at and see if the resultant DF is more likely
                theByte  ^= theErrs;
                thisDF    = (theByte >> 3) & 0x1f;
                thisDFbit = (1 << thisDF);
                // if this DF any more likely?
                if (validDFbits & thisDFbit) {
                    // Yep, more likely, so update the main message 
                    msg[0] = theByte;
                    errors--; // decrease the error count so we attempt to use the modified DF.
                }
            }
        }

        // snr = 5 * 20log10(sigLevel / noiseLevel)         (in units of 0.2dB)
        //     = 100log10(sigLevel) - 100log10(noiseLevel)

        while (sigLevel > 65535 || noiseLevel > 65535) {
            sigLevel >>= 1;
            noiseLevel >>= 1;
        }
        snr = Modes.log10lut[sigLevel] - Modes.log10lut[noiseLevel];

        // When we reach this point, if error is small, and the signal strength is large enough
        // we may have a Mode S message on our hands. It may still be broken and the CRC may not 
        // be correct, but this can be handled by the next layer.
        if ( (msglen) 
          && ((2 * snr) > (int) (MODES_MSG_SQUELCH_DB * 10))
          && (errors      <= MODES_MSG_ENCODER_ERRS) ) {
            int result;

            // Set initial mm structure details
            mm.timestampMsg = mag->sampleTimestamp + (j*6);

            // compute message receive time as block-start-time + difference in the 12MHz clock
            mm.sysTimestampMsg = mag->sysTimestamp; // start of block time
            mm.sysTimestampMsg.tv_nsec += receiveclock_ns_elapsed(mag->sampleTimestamp, mm.timestampMsg);
            normalize_timespec(&mm.sysTimestampMsg);

            mm.signalLevel = (365.0*60 + sigLevel + noiseLevel) * (365.0*60 + sigLevel + noiseLevel) / MAX_POWER / 60 / 60;

            // Decode the received message
            result = decodeModesMessage(&mm, msg);
            if (result < 0) {
                message_ok = 0;
                if (result == -1)
                    Modes.stats_current.demod_rejected_unknown_icao++;
                else
                    Modes.stats_current.demod_rejected_bad++;
            } else {
                message_ok = 1;
                Modes.stats_current.demod_accepted[mm.correctedbits]++;
            }

            // Update statistics

            // Output debug mode info if needed
            if (use_correction) {
                if (Modes.debug & MODES_DEBUG_DEMOD)
                    dumpRawMessage("Demodulated with 0 errors", msg, m, j);
                else if (Modes.debug & MODES_DEBUG_BADCRC &&
                         mm.msgtype == 17 &&
                         (!message_ok || mm.correctedbits > 0))
                    dumpRawMessage("Decoded with bad CRC", msg, m, j);
                else if (Modes.debug & MODES_DEBUG_GOODCRC &&
                         message_ok && 
                         mm.correctedbits == 0)
                    dumpRawMessage("Decoded with good CRC", msg, m, j);
            }

            // Skip this message if we are sure it's fine
            if (message_ok) {
                j += (MODES_PREAMBLE_US+msglen)*2 - 1;

                // Pass data to the next layer
                useModesMessage(&mm);
            }
        } else {
            message_ok = 0;
            if (Modes.debug & MODES_DEBUG_DEMODERR && use_correction) {
                printf("The following message has %d demod errors\n", errors);
                dumpRawMessage("Demodulated with errors", msg, m, j);
            }
        }

        // Retry with phase correction if enabled, necessary and possible.
        if (Modes.phase_enhance && (!message_ok || mm.correctedbits > 0) && !use_correction && j && detectOutOfPhase(pPreamble)) {
            use_correction = 1; j--;
        } else {
            use_correction = 0; 
        }
    }
}

