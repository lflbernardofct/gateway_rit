/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MEEC/MERSIM - FCT NOVA  2026/2027
 *
 * proxy_thread.h
 *
 * Header file of functions that implement the proxy threads, which bind IPv4 clients to IPv6 servers
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#ifndef PROXY_THREAD_H_
#define PROXY_THREAD_H_

#define SLOW_SLEEPTIME	500000	// Sleep time between reads and writes in slow sending
#define FILE_BUFLEN 65536		// Buffer size used to transmit data


// ** TASK 3.3 **
// Status values of a proxy thread
// typedef enum {  ... } thread_status;


// Thread state
typedef struct thread_state {
	int sock4;				// socket to TCP IPv4 client
	struct in6_addr cli_ip; // IPv4 address
	ushort cli_port;		// IPv4 port
	struct Query *q;		// Pointer to Query descriptor

	pthread_t tid;			// Thread id

	char *filename;			// filename requested
	u_int32_t seq;			// Sequence number
	int sock6;				// socket to IPv6 server

// you can add more elements to this structure if you need ...
	// ** TASK 3.3 **
	// thread_status status;	// Status of the thread

	struct thread_state *self;	// wealth checking self-pointer
} thread_state;


/******************************************\
|* Functions that handle the thread list  *|
\******************************************/

// Create a new thread state object
thread_state *new_thread_state(int sock4, struct sockaddr_in6 *cli_addr);
// Update the information about the IPv6 server
void update_thread_state(thread_state *pt, int sock6, const char *fname, u_int32_t seq);
// Search for thread_state descriptor in plist using the filename and sequence number
thread_state *locate_state_in_plist(const char *filename, u_int32_t seq);

// Free a thread state object, removing it from the list, clearing all info from the GUI
// and freeing all memory previously allocated
void free_thread_state(thread_state *pt, gboolean called_from_GUI);
// Stop a thread identified by the thread_state
void stop_thread_by_id(thread_state *pt);
// Stop a thread identified by the filename and the sequence number
gboolean stop_thread(const char *filename, u_int32_t seq);
// Close all threads
void close_all_threads(gboolean called_from_GUI);


/*****************************************************************************************\
|* Functions that implement the proxy and handle the communication between IPv4 and IPv6 *|
\*****************************************************************************************/

// Create a connection and return the socket
int connect_to_ipv6_server(const char *ip, uint port);
// Connect to one file server, cycling through all hits received
int connect_to_file_server(thread_state *state, const char *filename, u_int32_t seq);
// Function that implements the thread function:
//		it implements all communications between client IPv4 and server IPv6
//		ptr - pointer to the thread state object
void *proxy_function(void *ptr);

#endif /* PROXY_THREAD_H_ */
