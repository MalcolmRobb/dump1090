// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// track.c: aircraft state tracking
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

/* #define DEBUG_CPR_CHECKS */

//
// Return a new aircraft structure for the linked list of tracked
// aircraft
//
struct aircraft *trackCreateAircraft(struct modesMessage *mm) {
    static struct aircraft zeroAircraft;
    struct aircraft *a = (struct aircraft *) malloc(sizeof(*a));
    int i;

    // Default everything to zero/NULL
    *a = zeroAircraft;

    // Now initialise things that should not be 0/NULL to their defaults
    a->addr = mm->addr;
    for (i = 0; i < 8; ++i)
        a->signalLevel[i] = mm->signalLevel;  // First time, initialise everything
                                              // to the first signal strength

    // mm->msgtype 32 is used to represent Mode A/C. These values can never change, so 
    // set them once here during initialisation, and don't bother to set them every 
    // time this ModeA/C is received again in the future
    if (mm->msgtype == 32) {
        int modeC      = ModeAToModeC(mm->modeA | mm->fs);
        a->modeACflags = MODEAC_MSG_FLAG;
        if (modeC < -12) {
            a->modeACflags |= MODEAC_MSG_MODEA_ONLY;
        } else {
            mm->altitude = modeC * 100;
            mm->bFlags  |= MODES_ACFLAGS_ALTITUDE_VALID;
        }
    }

    // Copy the first message so we can emit it later when a second message arrives.
    a->first_message = *mm;

    Modes.stats_current.unique_aircraft++;

    return (a);
}

//
//=========================================================================
//
// Return the aircraft with the specified address, or NULL if no aircraft
// exists with this address.
//
struct aircraft *trackFindAircraft(uint32_t addr) {
    struct aircraft *a = Modes.aircrafts;

    while(a) {
        if (a->addr == addr) return (a);
        a = a->next;
    }
    return (NULL);
}

//
// CPR position updating
//

// Distance between points on a spherical earth.
// This has up to 0.5% error because the earth isn't actually spherical
// (but we don't use it in situations where that matters)
static double greatcircle(double lat0, double lon0, double lat1, double lon1)
{
    lat0 = lat0 * M_PI / 180.0;
    lon0 = lon0 * M_PI / 180.0;
    lat1 = lat1 * M_PI / 180.0;
    lon1 = lon1 * M_PI / 180.0;

    // avoid NaN
    if (fabs(lat0 - lat1) < 0.0001 && fabs(lon0 - lon1) < 0.0001)
        return 0.0;

    return 6371e3 * acos(sin(lat0) * sin(lat1) + cos(lat0) * cos(lat1) * cos(fabs(lon0 - lon1)));
}

static void update_range_histogram(double lat, double lon)
{
    if (Modes.stats_range_histo && (Modes.bUserFlags & MODES_USER_LATLON_VALID)) {
        double range = greatcircle(Modes.fUserLat, Modes.fUserLon, lat, lon);
        int bucket = round(range / Modes.maxRange * RANGE_BUCKET_COUNT);

        if (bucket < 0)
            bucket = 0;
        else if (bucket >= RANGE_BUCKET_COUNT)
            bucket = RANGE_BUCKET_COUNT-1;

        ++Modes.stats_current.range_histogram[bucket];
    }
}

// return true if it's OK for the aircraft to have travelled from its last known position
// to a new position at (lat,lon,surface) at a time of now.
static int speed_check(struct aircraft *a, struct modesMessage *mm, double lat, double lon, uint64_t now, int surface)
{
    uint64_t elapsed;
    double distance;
    double range;
    int speed;
    int inrange;

    if (!(a->bFlags & MODES_ACFLAGS_LATLON_VALID))
        return 1; // no reference, assume OK

    elapsed = now - a->seenLatLon;

    if ((mm->bFlags & MODES_ACFLAGS_SPEED_VALID) && (a->bFlags & MODES_ACFLAGS_SPEED_VALID))
        speed = (mm->velocity + a->speed) / 2;
    else if (mm->bFlags & MODES_ACFLAGS_SPEED_VALID)
        speed = mm->velocity;
    else if ((a->bFlags & MODES_ACFLAGS_SPEED_VALID) && (now - a->seenSpeed) < 30000)
        speed = a->speed;
    else
        speed = surface ? 100 : 600; // guess

    // Work out a reasonable speed to use:
    //  current speed + 1/3
    //  surface speed min 20kt, max 150kt
    //  airborne speed min 200kt, no max
    speed = speed * 4 / 3;
    if (surface) {
        if (speed < 20)
            speed = 20;
        if (speed > 150)
            speed = 150;
    } else {
        if (speed < 200)
            speed = 200;
    }

    // 100m (surface) or 500m (airborne) base distance to allow for minor errors,
    // plus distance covered at the given speed for the elapsed time + 1 second.
    range = (surface ? 0.1e3 : 0.5e3) + ((elapsed + 1000.0) / 1000.0) * (speed * 1852.0 / 3600.0);

    // find actual distance
    distance = greatcircle(a->lat, a->lon, lat, lon);

    inrange = (distance <= range);
#ifdef DEBUG_CPR_CHECKS
    if (!inrange) {
        fprintf(stderr, "Speed check failed: %06x: %.3f,%.3f -> %.3f,%.3f in %.1f seconds, max speed %d kt, range %.1fkm, actual %.1fkm\n",
                a->addr, a->lat, a->lon, lat, lon, elapsed/1000.0, speed, range/1000.0, distance/1000.0);
    }
#endif

    return inrange;
}

static int doGlobalCPR(struct aircraft *a, struct modesMessage *mm, uint64_t now, double *lat, double *lon, unsigned *nuc)
{
    int result;
    int fflag = (mm->bFlags & MODES_ACFLAGS_LLODD_VALID) != 0;
    int surface = (mm->bFlags & MODES_ACFLAGS_AOG) != 0;

    *nuc = (a->even_cprnuc < a->odd_cprnuc ? a->even_cprnuc : a->odd_cprnuc); // worst of the two positions

    if (surface) {
        // surface global CPR
        // find reference location
        double reflat, reflon;

        if (a->bFlags & MODES_ACFLAGS_LATLON_REL_OK) { // Ok to try aircraft relative first
            reflat = a->lat;
            reflon = a->lon;
            if (a->pos_nuc < *nuc)
                *nuc = a->pos_nuc;
        } else if (Modes.bUserFlags & MODES_USER_LATLON_VALID) {
            reflat = Modes.fUserLat;
            reflon = Modes.fUserLon;
        } else {
            // No local reference, give up
            return (-1);
        }

        result = decodeCPRsurface(reflat, reflon,
                                  a->even_cprlat, a->even_cprlon,
                                  a->odd_cprlat, a->odd_cprlon,
                                  fflag,
                                  lat, lon);
    } else {
        // airborne global CPR
        result = decodeCPRairborne(a->even_cprlat, a->even_cprlon,
                                   a->odd_cprlat, a->odd_cprlon,
                                   fflag,
                                   lat, lon);
    }

    if (result < 0) {
#ifdef DEBUG_CPR_CHECKS
        if (mm->bFlags & MODES_ACFLAGS_FROM_MLAT) {
            fprintf(stderr, "CPR: decode failure from MLAT (%06X) (%d).\n", a->addr, result);
            fprintf(stderr, "  even: %d %d   odd: %d %d  fflag: %s\n",
                    a->even_cprlat, a->even_cprlon,
                    a->odd_cprlat, a->odd_cprlon,
                    fflag ? "odd" : "even");
        }
#endif
        return result;
    }

    // for mlat results, accept it unquestioningly
    if (mm->bFlags & MODES_ACFLAGS_FROM_MLAT)
        return result;

    // check max range
    if (Modes.maxRange > 0 && (Modes.bUserFlags & MODES_USER_LATLON_VALID)) {
        double range = greatcircle(Modes.fUserLat, Modes.fUserLon, *lat, *lon);
        if (range > Modes.maxRange) {
#ifdef DEBUG_CPR_CHECKS
            fprintf(stderr, "Global range check failed: %06x: %.3f,%.3f, max range %.1fkm, actual %.1fkm\n",
                    a->addr, *lat, *lon, Modes.maxRange/1000.0, range/1000.0);
#endif

            Modes.stats_current.cpr_global_range_checks++;
            return (-2); // we consider an out-of-range value to be bad data
        }
    }

    // check speed limit
    if ((a->bFlags & MODES_ACFLAGS_LATLON_VALID) && a->pos_nuc >= *nuc && !speed_check(a, mm, *lat, *lon, now, surface)) {
        Modes.stats_current.cpr_global_speed_checks++;
        return -2;
    }

    return result;
}

static int doLocalCPR(struct aircraft *a, struct modesMessage *mm, uint64_t now, double *lat, double *lon, unsigned *nuc)
{
    // relative CPR
    // find reference location
    double reflat, reflon;
    double range_limit = 0;
    int result;
    int fflag = (mm->bFlags & MODES_ACFLAGS_LLODD_VALID) != 0;
    int surface = (mm->bFlags & MODES_ACFLAGS_AOG) != 0;

    *nuc = mm->nuc_p;

    if (a->bFlags & MODES_ACFLAGS_LATLON_REL_OK) {
        reflat = a->lat;
        reflon = a->lon;

        if (a->pos_nuc < *nuc)
            *nuc = a->pos_nuc;
    } else if (!surface && (Modes.bUserFlags & MODES_USER_LATLON_VALID)) {
        reflat = Modes.fUserLat;
        reflon = Modes.fUserLon;
        
        // The cell size is at least 360NM, giving a nominal
        // max range of 180NM (half a cell).
        //
        // If the receiver range is more than half a cell
        // then we must limit this range further to avoid
        // ambiguity. (e.g. if we receive a position report
        // at 200NM distance, this may resolve to a position
        // at (200-360) = 160NM in the wrong direction)

        if (Modes.maxRange <= 1852*180) {
            range_limit = Modes.maxRange;
        } else if (Modes.maxRange <= 1852*360) {
            range_limit = (1852*360) - Modes.maxRange;
        } else {
            return (-1); // Can't do receiver-centered checks at all
        }
    } else {
        // No local reference, give up
        return (-1);
    }

    result = decodeCPRrelative(reflat, reflon,
                               mm->raw_latitude,
                               mm->raw_longitude,
                               fflag, surface,
                               lat, lon);
    if (result < 0)
        return result;

    // check range limit
    if (range_limit > 0) {
        double range = greatcircle(reflat, reflon, *lat, *lon);
        if (range > range_limit) {
            Modes.stats_current.cpr_local_range_checks++;
            return (-1);
        }
    }

    // check speed limit
    if ((a->bFlags & MODES_ACFLAGS_LATLON_VALID) && a->pos_nuc >= *nuc && !speed_check(a, mm, *lat, *lon, now, surface)) {
        Modes.stats_current.cpr_local_speed_checks++;
        return -1;
    }

    return 0;
}

static void updatePosition(struct aircraft *a, struct modesMessage *mm, uint64_t now)
{
    int location_result = -1;
    int max_elapsed;
    double new_lat = 0, new_lon = 0;
    unsigned new_nuc = 0;

    if (mm->bFlags & MODES_ACFLAGS_AOG)
        ++Modes.stats_current.cpr_surface;
    else
        ++Modes.stats_current.cpr_airborne;

    if (mm->bFlags & MODES_ACFLAGS_AOG) {
        // Surface: 25 seconds if >25kt or speed unknown, 50 seconds otherwise

        if ((mm->bFlags & MODES_ACFLAGS_SPEED_VALID) && mm->velocity <= 25)
            max_elapsed = 50000;
        else
            max_elapsed = 25000;
    } else {
        // Airborne: 10 seconds
        max_elapsed = 10000;
    }

    if (mm->bFlags & MODES_ACFLAGS_LLODD_VALID) {
        a->odd_cprnuc  = mm->nuc_p;
        a->odd_cprlat  = mm->raw_latitude;
        a->odd_cprlon  = mm->raw_longitude;
        a->odd_cprtime = now;
    } else {
        a->even_cprnuc  = mm->nuc_p;
        a->even_cprlat  = mm->raw_latitude;
        a->even_cprlon  = mm->raw_longitude;
        a->even_cprtime = now;
    }

    // If we have enough recent data, try global CPR
    if (((mm->bFlags | a->bFlags) & MODES_ACFLAGS_LLEITHER_VALID) == MODES_ACFLAGS_LLBOTH_VALID && abs((int)(a->even_cprtime - a->odd_cprtime)) <= max_elapsed) {
        location_result = doGlobalCPR(a, mm, now, &new_lat, &new_lon, &new_nuc);

        if (location_result == -2) {
#ifdef DEBUG_CPR_CHECKS
            if (mm->bFlags & MODES_ACFLAGS_FROM_MLAT) {
                fprintf(stderr, "CPR failure from MLAT (%06X).\n", a->addr);
            }
#endif
            // Global CPR failed because the position produced implausible results.
            // This is bad data. Discard both odd and even messages and wait for a fresh pair.
            // Also disable aircraft-relative positions until we have a new good position (but don't discard the
            // recorded position itself)
            Modes.stats_current.cpr_global_bad++;
            a->bFlags &= ~(MODES_ACFLAGS_LATLON_REL_OK | MODES_ACFLAGS_LLODD_VALID | MODES_ACFLAGS_LLEVEN_VALID);

            // Also discard the current message's data as it is suspect - we don't want
            // to update any of the aircraft state from this.
            mm->bFlags &= ~(MODES_ACFLAGS_LATLON_VALID | MODES_ACFLAGS_LLODD_VALID | MODES_ACFLAGS_LLEVEN_VALID |
                            MODES_ACFLAGS_ALTITUDE_VALID |
                            MODES_ACFLAGS_SPEED_VALID |
                            MODES_ACFLAGS_HEADING_VALID |
                            MODES_ACFLAGS_NSEWSPD_VALID |
                            MODES_ACFLAGS_VERTRATE_VALID |
                            MODES_ACFLAGS_AOG_VALID |
                            MODES_ACFLAGS_AOG);
            return;
        } else if (location_result == -1) {
#ifdef DEBUG_CPR_CHECKS
            if (mm->bFlags & MODES_ACFLAGS_FROM_MLAT) {
                fprintf(stderr, "CPR skipped from MLAT (%06X).\n", a->addr);
            }
#endif
            // No local reference for surface position available, or the two messages crossed a zone.
            // Nonfatal, try again later.
            Modes.stats_current.cpr_global_skipped++;
        } else {
            Modes.stats_current.cpr_global_ok++;
        }
    }

    // Otherwise try relative CPR.
    if (location_result == -1) {
        location_result = doLocalCPR(a, mm, now, &new_lat, &new_lon, &new_nuc);

        if (location_result == -1) {
            Modes.stats_current.cpr_local_skipped++;
        } else {
            Modes.stats_current.cpr_local_ok++;
            if (a->bFlags & MODES_ACFLAGS_LATLON_REL_OK)
                Modes.stats_current.cpr_local_aircraft_relative++;
            else
                Modes.stats_current.cpr_local_receiver_relative++;
            mm->bFlags |= MODES_ACFLAGS_REL_CPR_USED;
        }
    }

    if (location_result == 0) {
        // If we sucessfully decoded, back copy the results to mm so that we can print them in list output
        mm->bFlags |= MODES_ACFLAGS_LATLON_VALID;
        mm->fLat    = new_lat;
        mm->fLon    = new_lon;

        // Update aircraft state
        a->bFlags |= (MODES_ACFLAGS_LATLON_VALID | MODES_ACFLAGS_LATLON_REL_OK);
        a->lat = new_lat;
        a->lon = new_lon;
        a->pos_nuc = new_nuc;
        a->seenLatLon      = a->seen;

        update_range_histogram(new_lat, new_lon);
    }
}

//
//=========================================================================
//
// Receive new messages and update tracked aircraft state
//

struct aircraft *trackUpdateFromMessage(struct modesMessage *mm)
{
    struct aircraft *a;
    uint64_t now = mstime();

    // Lookup our aircraft or create a new one
    a = trackFindAircraft(mm->addr);
    if (!a) {                              // If it's a currently unknown aircraft....
        a = trackCreateAircraft(mm);       // ., create a new record for it,
        a->next = Modes.aircrafts;         // .. and put it at the head of the list
        Modes.aircrafts = a;
    }

    a->signalLevel[a->messages & 7] = mm->signalLevel;// replace the 8th oldest signal strength
    a->seen      = now;
    a->messages++;

    // if the Aircraft has landed or taken off since the last message, clear the even/odd CPR flags
    if ((mm->bFlags & MODES_ACFLAGS_AOG_VALID) && ((a->bFlags ^ mm->bFlags) & MODES_ACFLAGS_AOG)) {
        a->bFlags &= ~(MODES_ACFLAGS_LLBOTH_VALID | MODES_ACFLAGS_AOG);
    }

    // If we've got a new cprlat or cprlon
    if (mm->bFlags & MODES_ACFLAGS_LLEITHER_VALID) {
        updatePosition(a, mm, now);
    }

    // If a (new) CALLSIGN has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_CALLSIGN_VALID) {
        memcpy(a->flight, mm->flight, sizeof(a->flight));
    }

    // If a (new) ALTITUDE has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_ALTITUDE_VALID) {
        if ( (a->modeCcount)                   // if we've a modeCcount already
          && (a->altitude  != mm->altitude ) ) // and Altitude has changed
//        && (a->modeC     != mm->modeC + 1)   // and Altitude not changed by +100 feet
//        && (a->modeC + 1 != mm->modeC    ) ) // and Altitude not changes by -100 feet
            {
            a->modeCcount   = 0;               //....zero the hit count
            a->modeACflags &= ~MODEAC_MSG_MODEC_HIT;
            }

        // If we received an altitude in a (non-mlat) DF17/18 squitter recently, ignore
        // DF0/4/16/20 altitudes as single-bit errors can attribute them to the wrong
        // aircraft
        if ((a->bFlags & ~a->mlatFlags & MODES_ACFLAGS_ALTITUDE_VALID) &&
            (now - a->seenAltitude) < 15000 &&
            (a->bFlags & ~a->mlatFlags & MODES_ACFLAGS_LATLON_VALID) &&
            (now - a->seenLatLon) < 15000 &&
            mm->msgtype != 17 &&
            mm->msgtype != 18) {
            Modes.stats_current.suppressed_altitude_messages++;
        } else {
            a->altitude = mm->altitude;
            a->modeC    = (mm->altitude + 49) / 100;
            a->seenAltitude = now;
        }
    }

    // If a (new) SQUAWK has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_SQUAWK_VALID) {
        if (a->modeA != mm->modeA) {
            a->modeAcount   = 0; // Squawk has changed, so zero the hit count
            a->modeACflags &= ~MODEAC_MSG_MODEA_HIT;
        }
        a->modeA = mm->modeA;
    }

    // If a (new) HEADING has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_HEADING_VALID) {
        a->track = mm->heading;
        a->seenTrack = now;
    }

    // If a (new) SPEED has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_SPEED_VALID) {
        a->speed = mm->velocity;
        a->seenSpeed = now;
    }

    // If a (new) Vertical Descent rate has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_VERTRATE_VALID) {
        a->vert_rate = mm->vert_rate;
    }

    // If a (new) category has been received, copy it to the aircraft structure
    if (mm->bFlags & MODES_ACFLAGS_CATEGORY_VALID) {
        a->category = mm->category;
    }

    // Update the aircrafts a->bFlags to reflect the newly received mm->bFlags;
    a->bFlags |= mm->bFlags;

    // Update mlat flags. The mlat flags indicate which bits in bFlags
    // were last set based on a mlat-derived message.
    if (mm->bFlags & MODES_ACFLAGS_FROM_MLAT)
        a->mlatFlags = (a->mlatFlags & a->bFlags) | mm->bFlags;
    else
        a->mlatFlags = (a->mlatFlags & a->bFlags) & ~mm->bFlags;

    if (mm->msgtype == 32) {
        int flags = a->modeACflags;
        if ((flags & (MODEAC_MSG_MODEC_HIT | MODEAC_MSG_MODEC_OLD)) == MODEAC_MSG_MODEC_OLD) {
            //
            // This Mode-C doesn't currently hit any known Mode-S, but it used to because MODEAC_MSG_MODEC_OLD is
            // set  So the aircraft it used to match has either changed altitude, or gone out of our receiver range
            //
            // We've now received this Mode-A/C again, so it must be a new aircraft. It could be another aircraft
            // at the same Mode-C altitude, or it could be a new airctraft with a new Mods-A squawk.
            //
            // To avoid masking this aircraft from the interactive display, clear the MODEAC_MSG_MODES_OLD flag
            // and set messages to 1;
            //
            a->modeACflags = flags & ~MODEAC_MSG_MODEC_OLD;
            a->messages    = 1;
        }
    }

    return (a);
}

//
// Periodic updates of tracking state
//

//
//=========================================================================
//
// Periodically search through the list of known Mode-S aircraft and tag them if this
// Mode A/C  matches their known Mode S Squawks or Altitudes(+/- 50feet).
//
// A Mode S equipped aircraft may also respond to Mode A and Mode C SSR interrogations.
// We can't tell if this is a Mode A or C, so scan through the entire aircraft list
// looking for matches on Mode A (squawk) and Mode C (altitude). Flag in the Mode S
// records that we have had a potential Mode A or Mode C response from this aircraft. 
//
// If an aircraft responds to Mode A then it's highly likely to be responding to mode C 
// too, and vice verca. Therefore, once the mode S record is tagged with both a Mode A
// and a Mode C flag, we can be fairly confident that this Mode A/C frame relates to that
// Mode S aircraft.
//
// Mode C's are more likely to clash than Mode A's; There could be several aircraft 
// cruising at FL370, but it's less likely (though not impossible) that there are two 
// aircraft on the same squawk. Therefore, give precidence to Mode A record matches
//
// Note : It's theoretically possible for an aircraft to have the same value for Mode A 
// and Mode C. Therefore we have to check BOTH A AND C for EVERY S.
//
static void trackUpdateAircraftModeA(struct aircraft *a)
{
    struct aircraft *b = Modes.aircrafts;

    while(b) {
        if ((b->modeACflags & MODEAC_MSG_FLAG) == 0) {  // skip any fudged ICAO records 

            // If both (a) and (b) have valid squawks...
            if ((a->bFlags & b->bFlags) & MODES_ACFLAGS_SQUAWK_VALID) {
                // ...check for Mode-A == Mode-S Squawk matches
                if (a->modeA == b->modeA) { // If a 'real' Mode-S ICAO exists using this Mode-A Squawk
                    b->modeAcount   = a->messages;
                    b->modeACflags |= MODEAC_MSG_MODEA_HIT;
                    a->modeACflags |= MODEAC_MSG_MODEA_HIT;
                    if ( (b->modeAcount > 0) &&
                       ( (b->modeCcount > 1)
                      || (a->modeACflags & MODEAC_MSG_MODEA_ONLY)) ) // Allow Mode-A only matches if this Mode-A is invalid Mode-C
                        {a->modeACflags |= MODEAC_MSG_MODES_HIT;}    // flag this ModeA/C probably belongs to a known Mode S                    
                }
            }

            // If both (a) and (b) have valid altitudes...
            if ((a->bFlags & b->bFlags) & MODES_ACFLAGS_ALTITUDE_VALID) {
                // ... check for Mode-C == Mode-S Altitude matches
                if (  (a->modeC     == b->modeC    )     // If a 'real' Mode-S ICAO exists at this Mode-C Altitude
                   || (a->modeC     == b->modeC + 1)     //          or this Mode-C - 100 ft
                   || (a->modeC + 1 == b->modeC    ) ) { //          or this Mode-C + 100 ft
                    b->modeCcount   = a->messages;
                    b->modeACflags |= MODEAC_MSG_MODEC_HIT;
                    a->modeACflags |= MODEAC_MSG_MODEC_HIT;
                    if ( (b->modeAcount > 0) &&
                         (b->modeCcount > 1) )
                        {a->modeACflags |= (MODEAC_MSG_MODES_HIT | MODEAC_MSG_MODEC_OLD);} // flag this ModeA/C probably belongs to a known Mode S                    
                }
            }
        }
        b = b->next;
    }
}
//
//=========================================================================
//
static void trackUpdateAircraftModeS()
{
    struct aircraft *a = Modes.aircrafts;

    while(a) {
        int flags = a->modeACflags;
        if (flags & MODEAC_MSG_FLAG) { // find any fudged ICAO records

            // clear the current A,C and S hit bits ready for this attempt
            a->modeACflags = flags & ~(MODEAC_MSG_MODEA_HIT | MODEAC_MSG_MODEC_HIT | MODEAC_MSG_MODES_HIT);

            trackUpdateAircraftModeA(a);  // and attempt to match them with Mode-S
        }
        a = a->next;
    }
}

//
//=========================================================================
//
// If we don't receive new nessages within TRACK_AIRCRAFT_TTL
// we remove the aircraft from the list.
//
static void trackRemoveStaleAircraft(uint64_t now)
{
    struct aircraft *a = Modes.aircrafts;
    struct aircraft *prev = NULL;
    
    while(a) {
        if ((now - a->seen) > TRACK_AIRCRAFT_TTL ||
            (a->messages == 1 && (now - a->seen) > TRACK_AIRCRAFT_ONEHIT_TTL)) {
            // Count aircraft where we saw only one message before reaping them.
            // These are likely to be due to messages with bad addresses.
            if (a->messages == 1)
                Modes.stats_current.single_message_aircraft++;

            // Remove the element from the linked list, with care
            // if we are removing the first element
            if (!prev) {
                Modes.aircrafts = a->next; free(a); a = Modes.aircrafts;
            } else {
                prev->next = a->next; free(a); a = prev->next;
            }
        } else {
            if ((a->bFlags & MODES_ACFLAGS_LATLON_VALID) && (now - a->seenLatLon) > TRACK_AIRCRAFT_POSITION_TTL) {
                /* Position is too old and no longer valid */
                a->bFlags &= ~(MODES_ACFLAGS_LATLON_VALID | MODES_ACFLAGS_LATLON_REL_OK);
            }
            prev = a; a = a->next;
        }
    }
}


//
// Entry point for periodic updates
//

void trackPeriodicUpdate()
{
    static uint64_t next_update;
    uint64_t now = mstime();

    // Only do updates once per second
    if (now >= next_update) {
        next_update = now + 1000;
        trackRemoveStaleAircraft(now);
        trackUpdateAircraftModeS();
    }
}
