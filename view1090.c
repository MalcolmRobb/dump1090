// view1090, a Mode S messages viewer for dump1090 devices.
//
// Copyright (C) 2014 by Malcolm Robb <Support@ATTAvionics.com>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#include "dump1090.h"
//
// ============================= Utility functions ==========================
//
void sigintHandler(int dummy) {
    MODES_NOTUSED(dummy);
    signal(SIGINT, SIG_DFL);  // reset signal handler - bit extra safety
    Modes.exit = 1;           // Signal to threads that we are done
}
//
// =============================== Terminal handling ========================
//
#ifndef _WIN32
// Get the number of rows after the terminal changes size.
int getTermRows() { 
    struct winsize w; 
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); 
    return (w.ws_row); 
} 

// Handle resizing terminal
void sigWinchCallback() {
    signal(SIGWINCH, SIG_IGN);
    Modes.interactive_rows = getTermRows();
    interactiveShowData();
    signal(SIGWINCH, sigWinchCallback); 
}
#else 
int getTermRows() { return MODES_INTERACTIVE_ROWS;}
#endif
//
// =============================== Initialization ===========================
//
void view1090InitConfig(void) {
    // Default everything to zero/NULL
    memset(&Modes,    0, sizeof(Modes));

    // Now initialise things that should not be 0/NULL to their defaults
    Modes.check_crc               = 1;
    Modes.interactive_rows        = getTermRows();
    Modes.interactive_display_ttl = MODES_INTERACTIVE_DISPLAY_TTL;
    Modes.interactive             = 1;
}
//
//=========================================================================
//
void view1090Init(void) {

    pthread_mutex_init(&Modes.data_mutex,NULL);
    pthread_cond_init(&Modes.data_cond,NULL);

#ifdef _WIN32
    if ( (!Modes.wsaData.wVersion) 
      && (!Modes.wsaData.wHighVersion) ) {
      // Try to start the windows socket support
      if (WSAStartup(MAKEWORD(2,1),&Modes.wsaData) != 0) 
        {
        fprintf(stderr, "WSAStartup returned Error\n");
        }
      }
#endif

    // Validate the users Lat/Lon home location inputs
    if ( (Modes.fUserLat >   90.0)  // Latitude must be -90 to +90
      || (Modes.fUserLat <  -90.0)  // and 
      || (Modes.fUserLon >  360.0)  // Longitude must be -180 to +360
      || (Modes.fUserLon < -180.0) ) {
        Modes.fUserLat = Modes.fUserLon = 0.0;
    } else if (Modes.fUserLon > 180.0) { // If Longitude is +180 to +360, make it -180 to 0
        Modes.fUserLon -= 360.0;
    }
    // If both Lat and Lon are 0.0 then the users location is either invalid/not-set, or (s)he's in the 
    // Atlantic ocean off the west coast of Africa. This is unlikely to be correct. 
    // Set the user LatLon valid flag only if either Lat or Lon are non zero. Note the Greenwich meridian 
    // is at 0.0 Lon,so we must check for either fLat or fLon being non zero not both. 
    // Testing the flag at runtime will be much quicker than ((fLon != 0.0) || (fLat != 0.0))
    Modes.bUserFlags &= ~MODES_USER_LATLON_VALID;
    if ((Modes.fUserLat != 0.0) || (Modes.fUserLon != 0.0)) {
        Modes.bUserFlags |= MODES_USER_LATLON_VALID;
    }

    // Prepare error correction tables
    modesChecksumInit(Modes.nfix_crc);
    icaoFilterInit();
}

//
// ================================ Main ====================================
//
void showHelp(void) {
    printf(
"-----------------------------------------------------------------------------\n"
"| view1090 ModeS Viewer       %45s |\n"
"-----------------------------------------------------------------------------\n"
  "--no-interactive         Disable interactive mode, print messages to stdout\n"
  "--interactive-rows <num> Max number of rows in interactive mode (default: 15)\n"
  "--interactive-ttl <sec>  Remove from list if idle for <sec> (default: 60)\n"
  "--interactive-rtl1090    Display flight table in RTL1090 format\n"
  "--modeac                 Enable decoding of SSR modes 3/A & 3/C\n"
  "--net-bo-ipaddr <IPv4>   TCP Beast output listen IPv4 (default: 127.0.0.1)\n"
  "--net-bo-port <port>     TCP Beast output listen port (default: 30005)\n"
  "--lat <latitude>         Reference/receiver latitide for surface posn (opt)\n"
  "--lon <longitude>        Reference/receiver longitude for surface posn (opt)\n"
  "--no-crc-check           Disable messages with broken CRC (discouraged)\n"
  "--no-fix                 Disable single-bits error correction using CRC\n"
  "--fix                    Enable single-bits error correction using CRC\n"
  "--aggressive             More CPU for more messages (two bits fixes, ...)\n"
  "--metric                 Use metric units (meters, km/h, ...)\n"
  "--show-only <addr>       Show only messages from the given ICAO on stdout\n"
  "--help                   Show this help\n",
  MODES_DUMP1090_VARIANT " " MODES_DUMP1090_VERSION
    );
}

//
//=========================================================================
//
int main(int argc, char **argv) {
    int j;
    struct client *c;
    struct net_service *s;
    char *bo_connect_ipaddr = "127.0.0.1";
    int bo_connect_port = MODES_NET_OUTPUT_BEAST_PORT;

    // Set sane defaults

    view1090InitConfig();
    signal(SIGINT, sigintHandler); // Define Ctrl/C handler (exit program)

    // Parse the command line options
    for (j = 1; j < argc; j++) {
        int more = ((j + 1) < argc); // There are more arguments

        if        (!strcmp(argv[j],"--net-bo-port") && more) {
            bo_connect_port = atoi(argv[++j]);
        } else if (!strcmp(argv[j],"--net-bo-ipaddr") && more) {
            bo_connect_ipaddr = argv[++j];
        } else if (!strcmp(argv[j],"--modeac")) {
            Modes.mode_ac = 1;
        } else if (!strcmp(argv[j],"--interactive-rows") && more) {
            Modes.interactive_rows = atoi(argv[++j]);
        } else if (!strcmp(argv[j],"--no-interactive")) {
            Modes.interactive = 0;
        } else if (!strcmp(argv[j],"--show-only") && more) {
            Modes.show_only = (uint32_t) strtoul(argv[++j], NULL, 16);
            Modes.interactive = 0;
        } else if (!strcmp(argv[j],"--interactive-ttl") && more) {
            Modes.interactive_display_ttl = (uint64_t)(1000 * atof(argv[++j]));
        } else if (!strcmp(argv[j],"--interactive-rtl1090")) {
            Modes.interactive = 1;
            Modes.interactive_rtl1090 = 1;
        } else if (!strcmp(argv[j],"--lat") && more) {
            Modes.fUserLat = atof(argv[++j]);
        } else if (!strcmp(argv[j],"--lon") && more) {
            Modes.fUserLon = atof(argv[++j]);
        } else if (!strcmp(argv[j],"--metric")) {
            Modes.metric = 1;
        } else if (!strcmp(argv[j],"--no-crc-check")) {
            Modes.check_crc = 0;
        } else if (!strcmp(argv[j],"--fix")) {
            Modes.nfix_crc = 1;
        } else if (!strcmp(argv[j],"--no-fix")) {
            Modes.nfix_crc = 0;
        } else if (!strcmp(argv[j],"--aggressive")) {
            Modes.nfix_crc = MODES_MAX_BITERRORS;
        } else if (!strcmp(argv[j],"--help")) {
            showHelp();
            exit(0);
        } else {
            fprintf(stderr, "Unknown or not enough arguments for option '%s'.\n\n", argv[j]);
            showHelp();
            exit(1);
        }
    }

#ifdef _WIN32
    // Try to comply with the Copyright license conditions for binary distribution
    if (!Modes.quiet) {showCopyright();}
#define MSG_DONTWAIT 0
#endif

#ifndef _WIN32
    // Setup for SIGWINCH for handling lines
    if (Modes.interactive) {signal(SIGWINCH, sigWinchCallback);}
#endif

    // Initialization
    view1090Init();
    modesInitNet();

    // Try to connect to the selected ip address and port. We only support *ONE* input connection which we initiate.here.
    s = makeBeastInputService();
    c = serviceConnect(s, bo_connect_ipaddr, bo_connect_port);
    if (!c) {
        fprintf(stderr, "Failed to connect to %s:%d: %s\n", bo_connect_ipaddr, bo_connect_port, Modes.aneterr);
        exit(1);
    }

    // Keep going till the user does something that stops us
    while (!Modes.exit) {
        icaoFilterExpire();
        trackPeriodicUpdate();
        modesNetPeriodicWork();

        if (Modes.interactive)
            interactiveShowData();

        if (s->connections == 0) {
            // lost input connection, try to reconnect
            usleep(1000000);
            c = serviceConnect(s, bo_connect_ipaddr, bo_connect_port);
            continue;
        }

        usleep(100000);
    }

    return (0);
}
//
//=========================================================================
//
