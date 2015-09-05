// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// track.h: aircraft state tracking prototypes
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

#ifndef DUMP1090_TRACK_H
#define DUMP1090_TRACK_H

/* Maximum age of tracked aircraft in milliseconds */
#define TRACK_AIRCRAFT_TTL 300000

/* Maximum age of a tracked aircraft with only 1 message received, in milliseconds */
#define TRACK_AIRCRAFT_ONEHIT_TTL 60000

/* Maximum validity of an aircraft position */
#define TRACK_AIRCRAFT_POSITION_TTL 60000

/* Structure used to describe the state of one tracked aircraft */
struct aircraft {
    uint32_t      addr;           // ICAO address
    char          flight[16];     // Flight number
    double        signalLevel[8]; // Last 8 Signal Amplitudes
    int           altitude;       // Altitude
    int           speed;          // Velocity
    int           track;          // Angle of flight
    int           vert_rate;      // Vertical rate.

    uint64_t      seen;           // Time (millis) at which the last packet was received
    uint64_t      seenLatLon;     // Time (millis) at which lat, lon was measured
    uint64_t      seenAltitude;   // Time (millis) at which altitude was measured
    uint64_t      seenSpeed;      // Time (millis) at which speed was measured
    uint64_t      seenTrack;      // Time (millis) at which track was measured

    int           mlatFlags;      // Data derived from mlat messages

    long          messages;       // Number of Mode S messages received
    int           modeA;          // Squawk
    int           modeC;          // Altitude
    long          modeAcount;     // Mode A Squawk hit Count
    long          modeCcount;     // Mode C Altitude hit Count
    int           modeACflags;    // Flags for mode A/C recognition

    int           fatsv_emitted_altitude;  // last FA emitted altitude
    int           fatsv_emitted_track;     // last FA emitted track
    int           fatsv_emitted_speed;     // last FA emitted speed
    uint64_t      fatsv_last_emitted;      // time (millis) aircraft was last FA emitted

    // Encoded latitude and longitude as extracted by odd and even CPR encoded messages
    uint64_t      odd_cprtime;
    int           odd_cprlat;
    int           odd_cprlon;
    unsigned      odd_cprnuc;

    uint64_t      even_cprtime;
    int           even_cprlat;
    int           even_cprlon;
    unsigned      even_cprnuc;

    double        lat, lon;       // Coordinated obtained from CPR encoded data
    unsigned      pos_nuc;        // NUCp of last computed position

    unsigned      category;       // Aircraft category A0 - D7 encoded as a single hex byte

    int           bFlags;         // Flags related to valid fields in this structure
    struct aircraft *next;        // Next aircraft in our linked list

    struct modesMessage first_message;  // A copy of the first message we received for this aircraft.
};



/* Update aircraft state from data in the provided mesage.
 * Return the tracked aircraft.
 */
struct modesMessage;
struct aircraft *trackUpdateFromMessage(struct modesMessage *mm);

/* Call periodically */
void trackPeriodicUpdate();

#endif
