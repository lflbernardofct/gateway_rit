/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MEEC/MERSIM - FCT NOVA  2026/2027
 *
 * callbacks_socket.c
 *
 * Functions that handle UDP/TCP socket management and UDP communication
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "sock.h"
#include "gui.h"
#include "callbacks.h"
#include "callbacks_socket.h"
#include "proxy_thread.h"

#ifdef DEBUG
#define debugstr(x)     g_print(x)
#else
#define debugstr(x)
#endif

gboolean active4 = FALSE; // TRUE if IPv4 multicast is on
gboolean active6 = FALSE; // TRUE if IPv6 multicast is on

guint query_timer_id = 0; // Timer event

int sockUDP4 = -1; // IPv4 UDP socket descriptor
u_short port_MCast4 = 0; // IPv4 Multicast port
const char *str_addr_MCast4 = NULL; // IPv4 multicast address
struct sockaddr_in6 addr_MCast4; // struct with destination data of IPv4 socket
struct ip_mreq imr_MCast4; // struct with IPv4 multicast data
GIOChannel *chanUDP4 = NULL; // GIO channel descriptor of socket UDPv4
guint chanUDP4_id = 0; // Channel number of socket UDPv4

int sockUDP6 = -1; // IPv6 UDP socket descriptor
u_short port_MCast6 = 0; // IPv6 Multicast port
const char *str_addr_MCast6 = NULL; // IPv6 multicast address
struct sockaddr_in6 addr_MCast6; // struct with destination data of IPv6 socket
struct ipv6_mreq imr_MCast6; // struct with IPv6 multicast data
GIOChannel *chanUDP6 = NULL; // GIO channel descriptor of socket UDPv6
guint chanUDP6_id = 0; // Channel number of socket UDPv6

int sockUDPq = -1;	// IPv6 UDP query socket descriptor (for sending queries)
GIOChannel *chanUDPq = NULL; // GIO channel descriptor for query socket
guint chanUDPq_id = 0;	// Channel number of query socket
u_short portUDPq = 0;	// Port number of query socket

int sockTCP= -1;		// TCP IPv4 socket descriptor
GIOChannel *chanTCP= NULL;	// GIO descriptor of TCP IPv4 socket
guint chanTCP_id= 0;	// GIO channel number of TCP IPV4 socket
u_short portTCP= 0;		// TCP IPv4 port


/* Local variables */
static char tmp_buf[8000];

/*****************************************\
|* Functions to write and read messages  *|
 \*****************************************/

// Write the QUERY message fields ('seq', 'filename') into buffer 'buf'
//    and returns the length in 'len'
gboolean write_query_message(char *buf, int *len, uint32_t seq, const char* filename) {
	if ((buf == NULL) || (len == NULL) || (filename == NULL))
		return FALSE;

	short int fnlen= strlen(filename)+1;
	unsigned char cod= MSG_QUERY;
	char *pt= buf;

	// write the QUERY code to the buffer
	WRITE_BUF(pt, &cod, sizeof(unsigned char));

	Log("write_query_message not implemented yet\n");
	// ** TASK 1.1 **: complete the code
	// ...
	// *len= ... length of the bytes written to buf ...;

	return TRUE;
}


// Read the QUERY message fields ('seq', 'filename') from buffer 'buf'
//    with length 'len'; returns TRUE if successful, or FALSE otherwise
gboolean read_query_message(char *buf, int len, uint32_t *seq, const char **filename) {
	if ((buf == NULL) || (seq == NULL) || (filename == NULL) || (len <= 9))
		return FALSE;

	unsigned char cod;
	char *pt= buf;
	short int fnlen;

	READ_BUF(pt, &cod, sizeof(unsigned char));
	if (cod != MSG_QUERY)
		return FALSE;
	READ_BUF(pt, seq, sizeof(uint32_t));
	READ_BUF(pt, &fnlen, sizeof(fnlen));
	if ((fnlen != len-7) || (strlen(pt) != fnlen-1))
		return FALSE;
	*filename= strdup(pt);
	return TRUE;
}


// Write the HIT message fields ('seq','filename','fhash','flen','sTCP_port','ip6/ip4')
//    into buffer 'buf' and returns the length in 'len'
gboolean write_hit_message(char *buf, int *len, uint32_t seq, const char* filename, unsigned long long flen,
							uint32_t fhash, unsigned short sTCP_port, gboolean is_ipv6,
							struct in6_addr *srvIP6, struct in_addr *srvIP4)
{
	char *pt= buf;
	short int templen;

	if ((buf == NULL) || (len == NULL) || (filename == NULL) || (sTCP_port == 0) ||
			(is_ipv6 && (srvIP6 == NULL)) || (!is_ipv6 && (srvIP4 == NULL)))
		return FALSE;

	templen= strlen(filename)+1;
	unsigned char cod= MSG_HIT;
	WRITE_BUF(pt, &cod, sizeof(unsigned char));
	WRITE_BUF(pt, &seq, sizeof(uint32_t));
	WRITE_BUF(pt, &templen, sizeof(templen));
	WRITE_BUF(pt, filename, templen);
	WRITE_BUF(pt, &flen, sizeof(unsigned long long));
	WRITE_BUF(pt, &fhash, sizeof(uint32_t));
	WRITE_BUF(pt, &sTCP_port, sizeof(unsigned short));
	if (is_ipv6) {
		WRITE_BUF(pt, srvIP6, sizeof(struct in6_addr));
	} else {
		WRITE_BUF(pt, srvIP4, sizeof(struct in_addr));
	}
	*len= pt-buf;
	return TRUE;
}


// Read the HIT message fields ('seq','filename','flen','fhash','sTCP_port','ip6/ip4') from buffer 'buf'
//    with length 'len'; returns TRUE if successful, or FALSE otherwise
gboolean read_hit_message(char *buf, int len, uint32_t *seq, char *filename, unsigned long long *flen,
							uint32_t *fhash, unsigned short *sTCP_port, gboolean is_ipv6,
							struct in6_addr *srvIP6, struct in_addr *srvIP4)
{
	if ((buf == NULL) || (seq == NULL) || (filename == NULL) || (flen == NULL) ||
			(fhash == NULL) || (sTCP_port == NULL) || (is_ipv6 && (srvIP6 == NULL)) ||
			(!is_ipv6 && (srvIP4 == NULL)))
		return FALSE;

	unsigned char cod;
	char *pt= buf;
	short int filenamelen;

	READ_BUF(pt, &cod, sizeof(unsigned char));
	if (cod != MSG_HIT)
		return FALSE;
	READ_BUF(pt, seq, sizeof(uint32_t));
	READ_BUF(pt, &filenamelen, sizeof(filenamelen));
	if (strlen(pt) != (filenamelen-1)) {
		fprintf(stderr, "Error in read_hit_message: Invalid filename length\n");
		return FALSE;
	}
	READ_BUF(pt, filename, filenamelen);
	READ_BUF(pt, flen, sizeof(unsigned long long));
	READ_BUF(pt, fhash, sizeof(uint32_t));
	READ_BUF(pt, sTCP_port, sizeof(unsigned short));
	if (is_ipv6) {
		READ_BUF(pt, srvIP6, sizeof(struct in6_addr));
	} else {
		READ_BUF(pt, srvIP4, sizeof(struct in_addr));
	}
	if (pt-buf != len) {
		fprintf(stderr, "Error in read_hit_message: Invalid total length\n");
		*filename= '\0';
		return FALSE;
	}
	return TRUE;
}


/*************************************************\
|* Socket callback and message sending functions *|
 \************************************************/

// Send a packet to an UDP IPv6 socket
static gboolean send_message6(int sock, struct in6_addr *ip, u_short port,
		const char *buf, int n) {
	struct sockaddr_in6 addr;

	assert((ip!=NULL) && (buf!=NULL) && (n>0));
	addr.sin6_family = AF_INET6;
	addr.sin6_flowinfo = 0;
	addr.sin6_port = htons(port);
	memcpy(&addr.sin6_addr, ip, sizeof(struct in6_addr));
	/* Send message. */
	if (sendto(sock, buf, n, 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("send_unicast6: Error sending datagram");
		return FALSE;
	}
	return TRUE;
}

// Send a packet to an UDP IPv4 socket
gboolean send_message4(struct in_addr *ip, u_short port,
		const char *buf, int n) {
	struct sockaddr_in addr;

	assert((ip!=NULL) && (buf!=NULL) && (n>0));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr, ip, sizeof(struct in_addr));
	/* Send message. */
	if (sendto(sockUDP4, buf, n, 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("send_unicast4: Error sending datagram");
		return FALSE;
	}
	return TRUE;
}


// Send a reply packet from the multicast socket
gboolean send_M6reply(struct in6_addr *ip, u_short port, const char *buf, int n) {
	assert(sockUDP6 > 0);
	return send_message6(sockUDP6, ip, port, buf, n);
}

// Send a packet to an UDP socket
gboolean send_unicast(struct in6_addr *ip, u_short port, const char *buf, int n) {
	assert(sockUDPq > 0);
	return send_message6(sockUDPq, ip, port, buf, n);
}

// Send packet to the IPv6/IPv4 Multicast address
gboolean send_multicast(const char *buf, int n, gboolean use_IPv6) {
	assert(sockUDPq > 0);
	if (use_IPv6) {
		assert(str_addr_MCast6 != NULL);
	} else {
		assert(str_addr_MCast4 != NULL);
	}
	assert((buf!=NULL) && (n>0));
	/* Send message. */
	const gchar *error_msg =
			use_IPv6 ?
					"Error sending datagram to IPv6 group" :
					"Error sending datagram to IPv4 group";
	if (sendto(sockUDPq, buf, n, 0,
			(struct sockaddr *) (use_IPv6 ? &addr_MCast6 : &addr_MCast4),
			sizeof(addr_MCast6)) < 0) {
		perror(error_msg);
		return FALSE;
	}
	return TRUE;
}

// Callback to receive connections at TCP socket
gboolean callback_connections_TCP(GIOChannel *source, GIOCondition condition,
		gpointer data) {

	if (!active || (sockTCP<0))
		return FALSE;

	if (condition & G_IO_IN) {
		// Received a new connection
		struct sockaddr_in6 server;
		int msgsock;
		socklen_t length = sizeof(server);

		msgsock = accept(sockTCP, (struct sockaddr *) &server, &length);
		if (msgsock == -1) {
			perror("accept");
			Log("accept failed - aborting\nPlease turn off the application!\n");

			return FALSE; // Turns callback off
		} else {
			sprintf(tmp_buf, "Received connection from %s - %d\n",
					addr_ipv6(&server.sin6_addr), ntohs(server.sin6_port));
			Log(tmp_buf);

			// Starts a thread to read the data from the socket
			return handle_new_connection(msgsock, &server);
		}

	} else if ((condition & G_IO_NVAL) || (condition & G_IO_ERR)) {
		Log("Detected error in a TCP server socket\nPlease, turn off the application\n");
		return FALSE; // Stops callback for receiving connections in the socket
	} else {
		assert(0); // Should never reach this line
		return FALSE; // Stops callback for receiving connections in the socket
	}
}

// Callback to receive data from UDP IPv6 unicast socket
gboolean callback_UDPUnicast_data(GIOChannel *source, GIOCondition condition,
		gpointer data) {
	static char buf[MESSAGE_MAX_LENGTH]; // buffer for reading data
	struct in6_addr ipv6;
	char ip_str[81];
	u_short port;
	int n;

	//	assert(window1 != NULL);
	if (!active) {
		debugstr("callback_UDPUnicast_data with active=FALSE\n");
		return FALSE;
	}
	if (condition & G_IO_IN) {
		// Receive packet //
		n = read_data_ipv6(sockUDPq, buf, MESSAGE_MAX_LENGTH, &ipv6, &port);
		strncpy(ip_str, addr_ipv6(&ipv6), sizeof(ip_str)-1);
		ip_str[sizeof(ip_str)-1]= '\0';
		if (n <= 0) {
			Log("Failed reading packet from unicast socket\n");
			return TRUE; // Continue waiting for more events
		} else {
			time_t tbuf;
			unsigned char m;
			char *pt;
			gboolean is_ipv4 = (strstr(ip_str, "::ffff:") != NULL);

			// Read data //
			pt = buf;
			READ_BUF(pt, &m, 1); // Reads type and advances pointer
			// Writes date and sender's data //
			time(&tbuf);
			g_print("%sReceived %d bytes (unicast) from %s#%hu - type %hhd\n",
					ctime(&tbuf), n, ip_str, port, m);
			switch (m) {
			case MSG_HIT:
				handle_Hit(buf, n, &ipv6, port, !is_ipv4);
				break;

			default:
				sprintf(tmp_buf,
						"Invalid packet type (%d) in unicast socket - ignored\n",
						(int) m);
				Log(tmp_buf);
			}
			return TRUE; // Keeps receiving more packets
		}
	} else if ((condition & G_IO_NVAL) || (condition & G_IO_ERR)) {
		Log("Error detected in UDP query socket\n");
		// Turns sockets off
		close_all(FALSE);
		// Closes the application
		gtk_main_quit();
		return FALSE; // Stops callback for receiving packets from socket
	} else {
		assert(0); // Should never reach this line
		return FALSE; // Stops callback for receiving packets from socket
	}
}

// Callback to receive data from UDP IPv6/IPv4 multicast sockets
//   *((int*)data) is equal to 6 for IPv6 and to 4 for IPv4
gboolean callback_UDPMulticast_data(GIOChannel *source, GIOCondition condition,
		gpointer data) {
	static char buf[MESSAGE_MAX_LENGTH]; // buffer for reading data
	struct in6_addr ipv6;
	struct in_addr ipv4;
	char ip_str[81];
	u_short port;
	int n;
	gboolean from_v6 = (*(int *) data == 6); // if TRUE comes from IPv6, else from IPv4

	if (!active) {
		debugstr("callback_UDP Multicast_data with active=FALSE\n");
		return FALSE;
	}
	if (condition & G_IO_IN) {
		// Receive packet //
		if (from_v6 && active6) {
			n = read_data_ipv6(sockUDP6, buf, MESSAGE_MAX_LENGTH, &ipv6, &port);
			strncpy(ip_str, addr_ipv6(&ipv6), sizeof(ip_str)-1);
			ip_str[sizeof(ip_str)-1]= '\0';
		} else if (!from_v6 && active4) {
			n = read_data_ipv4(sockUDP4, buf, MESSAGE_MAX_LENGTH, &ipv4, &port);
			strncpy(ip_str, addr_ipv4(&ipv4), sizeof(ip_str)-1);
			ip_str[sizeof(ip_str)-1]= '\0';
			if (!translate_ipv4_to_ipv6(ip_str, &ipv6)) {
				debugstr("Error in callback_UDPMulticast_data: converting ipv4 to ipv6\n");
				return FALSE;
			}
		} else {
			debugstr("Error in callback_UDPMulticast_data: no read");
			return FALSE;
			assert(active6 || active4);
		}
		if (n <= 0) {
			Log("Failed reading packet from multicast socket\n");
			return TRUE; // Continue waiting for more events
		} else {
			time_t tbuf;
			unsigned char m;
			char *pt;

			// Read data //
			pt = buf;
			READ_BUF(pt, &m, 1); // Reads type and advances pointer
			// Writes date and sender's data //
			time(&tbuf);
			g_print("%sReceived %d bytes (multicast) from %s#%hu - type %hhd\n",
					ctime(&tbuf), n, ip_str, port, m);
			switch (m) {
			case MSG_QUERY:
				handle_Query(buf, n, from_v6, &ipv6, &ipv4, port);
				break;

			default:
				sprintf(tmp_buf,
						"Invalid packet type (%d) in multicast socket - ignored\n",
						(int) m);
				Log(tmp_buf);
			}
			return TRUE; // Keeps receiving more packets
		}
	} else if ((condition & G_IO_NVAL) || (condition & G_IO_ERR)) {
		Log("Error detected in UDP socket\n");
		// Turns sockets off
		close_all(FALSE);
		// Closes the application
		gtk_main_quit();
		return FALSE; // Stops callback for receiving packets from socket
	} else {
		assert(0); // Should never reach this line
		return FALSE; // Stops callback for receiving packets from socket
	}
}


/******************************************\
|* Functions to initialize/close sockets  *|
 \******************************************/

// Close all UDP sockets
void close_sockUDP(void) {
	debugstr("close_sockUDP\n");

	// IPv4
	if (chanUDP4 != NULL) {
		remove_socket_from_mainloop(sockUDP4, chanUDP4_id, chanUDP4);
		chanUDP4 = NULL;
		// Closed socket
	} else {
		if (sockUDP4 > 0) {
			if (str_addr_MCast4 != NULL) {
				// Leaves the multicast group
				if (setsockopt(sockUDP4, IPPROTO_IP, IP_DROP_MEMBERSHIP,
						(char *) &imr_MCast4, sizeof(imr_MCast4)) == -1) {
					perror("Failed de-association to IPv4 multicast group");
					sprintf(tmp_buf,
							"Failed de-association to IPv4 multicast group (%hu)\n",
							sockUDP4);
					Log(tmp_buf);
				}
			}
			if (close(sockUDP4))
				perror("Error during close of IPv4 multicast socket");
		}
	}
	sockUDP4 = -1;
	str_addr_MCast4 = NULL;
	active4 = FALSE;

	// IPv6
	if (chanUDP6 != NULL) {
		remove_socket_from_mainloop(sockUDP6, chanUDP6_id, chanUDP6);
		chanUDP6 = NULL;
		// Closed socket
	} else {
		if (sockUDP6 > 0) {
			if (str_addr_MCast6 != NULL) {
				// Leaves the group
				if (setsockopt(sockUDP6, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
						(char *) &imr_MCast6, sizeof(imr_MCast6)) == -1) {
					perror("Failed de-association to IPv6 multicast group");
					sprintf(tmp_buf,
							"Failed de-association to IPv6 multicast group (%hu)\n",
							sockUDP6);
					Log(tmp_buf);
					/* NOTE: Kernel 2.4 has a bug - it does not support de-association of IPv6 groups! */
				}
			}
			if (close(sockUDP6))
				perror("Error during close of IPv6 multicast socket");
		}
	}
	sockUDP6 = -1;
	str_addr_MCast6 = NULL;
	active6 = FALSE;

	// Query socket
	if (chanUDPq != NULL) {
		remove_socket_from_mainloop(sockUDPq, chanUDPq_id, chanUDPq);
		chanUDPq = NULL;
		// Closed socket
	} else {
		if (sockUDPq > 0) {
			if (close(sockUDPq))
				perror("Error during close of IPv6 query socket");
		}
	}
	sockUDPq = -1;
	portUDPq = 0;
}

// Close TCP socket and free GIO resources
void close_sockTCP(void) {
	if (sockTCP > 0) {
		if (chanTCP != NULL) {
			remove_socket_from_mainloop(sockTCP, chanTCP_id, chanTCP);
			chanTCP_id = 0;
			chanTCP= NULL;
		}
		close(sockTCP);
		sockTCP = -1;
	}
	portTCP= 0;
}

// Create IPv4 UDP multicast socket, configure it, and register its callback
gboolean init_socket_udp4(u_short port_multicast, const char *addr_multicast) {
	static const int number4 = 4;

	if (active4 || !addr_multicast)
		return TRUE;

	// Prepares the multicast association data structure
	if (!get_IPv4(addr_multicast, &imr_MCast4.imr_multiaddr)) {
		return FALSE;
	}
	imr_MCast4.imr_interface.s_addr = htonl(INADDR_ANY);

	// IPv6 struct used to send data to the IPv4 group from an IPv6 socket
	addr_MCast4.sin6_port = htons(port_multicast);
	addr_MCast4.sin6_family = AF_INET6;
	addr_MCast4.sin6_flowinfo = 0;
	if (!translate_ipv4_to_ipv6(addr_multicast, &addr_MCast4.sin6_addr))
		return FALSE;

	// Creates the IPV4 UDP socket
	sockUDP4 = init_socket_ipv4(SOCK_DGRAM, port_multicast, TRUE); // Share port
	fprintf(stderr, "UDP4 = %d\n", sockUDP4);
	if (sockUDP4 < 0) {
		Log("Failed opening IPv4 UDP socket\n");
		return FALSE;
	}
	// Join the group
	if (setsockopt(sockUDP4, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			(char *) &imr_MCast4, sizeof(struct ip_mreq)) == -1) {
		perror("Failed association to IPv4 multicast group");
		Log("Failed association to IPv4 multicast group\n");
		return FALSE;
	}
	str_addr_MCast4 = addr_multicast; // Memorizes it is associated to a group

	// Configure the socket to receive an echo of the multicast packets sent by this application
	char loop = 1;
	setsockopt(sockUDP4, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

	// Regists the socket in the main loop of Gtk+
	if (!put_socket_in_mainloop(sockUDP4, (void *) &number4, &chanUDP4_id,
			&chanUDP4, G_IO_IN, callback_UDPMulticast_data)) {
		Log("Failed registration of UDPv4 socket at Gnome\n");
		close_sockUDP();
		return FALSE;
	}
	active4 = TRUE;
	return TRUE;
}

// Create IPv6 UDP multicast socket, configure it, and register its callback
gboolean init_socket_udp6(u_short port_multicast, const char *addr_multicast) {
	static const int number6 = 6;

	if (active6 || !addr_multicast)
		return TRUE;

	// Prepares the data structures
	if (!get_IPv6(addr_multicast, &addr_MCast6.sin6_addr)) {
		return FALSE;
	}
	addr_MCast6.sin6_port = htons(port_multicast);
	addr_MCast6.sin6_family = AF_INET6;
	addr_MCast6.sin6_flowinfo = 0;
	bcopy(&addr_MCast6.sin6_addr, &imr_MCast6.ipv6mr_multiaddr,
			sizeof(addr_MCast6.sin6_addr));
	imr_MCast6.ipv6mr_interface = 0;

	// Creates the IPV4 UDP socket
	sockUDP6 = init_socket_ipv6(SOCK_DGRAM, port_multicast, TRUE);
	fprintf(stderr, "UDP6 = %d\n", sockUDP6);
	if (sockUDP6 < 0) {
		Log("Failed opening IPv6 UDP socket\n");
		return FALSE;
	}
	// Join the multicast group
	if (setsockopt(sockUDP6, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			(char *) &imr_MCast6, sizeof(imr_MCast6)) == -1) {
		perror("Failed association to IPv6 multicast group");
		Log("Failed association to IPv6 multicast group\n");
		return FALSE;
	}
	str_addr_MCast6 = addr_multicast; // Memorizes it is associated to a group

	// Configure the socket to receive an echo of the multicast packets sent by this application
	char loop = 1;
	setsockopt(sockUDP6, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop));

	// Regists the socket in the main loop of Gtk+
	if (!put_socket_in_mainloop(sockUDP6, (void *) &number6, &chanUDP6_id,
			&chanUDP6, G_IO_IN, callback_UDPMulticast_data)) {
		Log("Failed registration of UDPv6 socket at Gnome\n");
		close_sockUDP();
		return FALSE;
	}
	active6 = TRUE;
	return TRUE;
}

// Create IPv6 TCP socket, configure it, and register its callback
gboolean init_server_socket_tcp6(void) {

	// Creates TCP socket
	sockTCP = init_socket_ipv6(SOCK_STREAM, 0, FALSE);
	if (sockTCP < 0) {
		Log("Failed opening IPv6 TCP server socket\n");
		return FALSE;
	}
	// Prepares the socket to receive connections
	if (listen(sockTCP, 0) < 0) {
		perror("Listen failed\n");
		Log("Listen failed\n");
		close(sockTCP);
		sockTCP= -1;
		return FALSE;
	}
	portTCP= get_portnumber(sockTCP);
	set_PortTCP(portTCP);

	// Regists the TCP socket in Gtk+ main loop
	if (!put_socket_in_mainloop(sockTCP, NULL, &chanTCP_id, &chanTCP, G_IO_IN,
			callback_connections_TCP)) {
		Log("Failed registration of TCPv6 server socket at Gnome\n");
		close(sockTCP);
		sockTCP= -1;
		return FALSE;
	}
	return TRUE;
}


// Initialize all sockets. Configures the following global variables:
// port_MCast - multicast port
// str_addr_MCast4/6 - IP multicast address
// imr_MCast4/6 - struct for association to to IP Multicast address
// addr_MCast4/6 - struct with UDP socket data for sending packets to the groups
gboolean init_sockets(u_short port4_multicast, const char *addr4_multicast,
		u_short port6_multicast, const char *addr6_multicast) {

	gboolean ok = init_socket_udp4(port4_multicast, addr4_multicast);
	ok |= init_socket_udp6(port6_multicast, addr6_multicast);
	if (!ok)
		return FALSE;
	// TCP server socket
	if (!init_server_socket_tcp6()) {
		close_sockUDP();
		return FALSE;
	}

	// Query socket	//////////////////////////////////////////////////////////
	sockUDPq = init_socket_ipv6(SOCK_DGRAM, 0, FALSE);
	if (sockUDPq < 0) {
		Log("Failed opening IPv6 UDP query socket\n");
		close_sockUDP();
		close_sockTCP();
		return FALSE;
	}
	portUDPq = get_portnumber(sockUDPq);

	// Regists the socket in the main loop of Gtk+
	if (!put_socket_in_mainloop(sockUDPq, NULL, &chanUDPq_id, &chanUDPq,
			G_IO_IN, callback_UDPUnicast_data)) {
		Log("Failed registration of query UDPv6 socket at Gnome\n");
		close_sockUDP();
		close_sockTCP();
		return FALSE;
	}

	return TRUE;
}

