// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// net_io.h: network handling.
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

#ifndef DUMP1090_NETIO_H
#define DUMP1090_NETIO_H

// Describes a networking service (group of connections)

struct modesMessage;
struct client;
struct net_service;
typedef int (*read_fn)(struct client *, char *);
typedef void (*heartbeat_fn)(struct net_service *);

// Describes one network service (a group of clients with common behaviour)
struct net_service {
    struct net_service* next;
    const char *descr;
    int listen_fd;

    int connections;     // number of active clients

    struct net_writer *writer; // shared writer state

    const char *read_sep;      // hander details for input data
    read_fn read_handler;
};

// Structure used to describe a networking client
struct client {
    struct client*  next;                // Pointer to next client
    int    fd;                           // File descriptor
    struct net_service *service;         // Service this client is part of
    int    buflen;                       // Amount of data on buffer
    char   buf[MODES_CLIENT_BUF_SIZE+1]; // Read buffer
};

// Common writer state for all output sockets of one type
struct net_writer {
    struct net_service *service; // owning service
    void *data;          // shared write buffer, sized MODES_OUT_BUF_SIZE
    int dataUsed;        // number of bytes of write buffer currently used
    uint64_t lastWrite;  // time of last write to clients
    heartbeat_fn send_heartbeat; // function that queues a heartbeat if needed
};

struct net_service *serviceInit(const char *descr, struct net_writer *writer, heartbeat_fn hb_handler, const char *sep, read_fn read_handler);
struct client *serviceConnect(struct net_service *service, char *addr, int port);
void serviceListen(struct net_service *service, char *bind_addr, int bind_port);
struct client *createSocketClient(struct net_service *service, int fd);
struct client *createGenericClient(struct net_service *service, int fd);

// view1090 / faup1090 want to create these themselves:
struct net_service *makeBeastInputService(void);
struct net_service *makeFatsvOutputService(void);

void modesInitNet(void);
void modesQueueOutput(struct modesMessage *mm);
void modesNetPeriodicWork(void);

// TODO: move these somewhere else
char *generateAircraftJson(const char *url_path, int *len);
char *generateStatsJson(const char *url_path, int *len);
char *generateReceiverJson(const char *url_path, int *len);
char *generateHistoryJson(const char *url_path, int *len);
void writeJsonToFile(const char *file, char * (*generator) (const char *,int*));

#endif
