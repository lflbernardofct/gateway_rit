/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MEEC/MERSIM - FCT NOVA  2026/2027
 *
 * callbacks.c
 *
 * Functions that handle main application logic for UDP communication, controlling query forwarding
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
#include <math.h>
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

/**********************\
|*  Global variables  *|
 \**********************/

gboolean active = FALSE; 	// TRUE if demo_gateway is active

GList *qlist = NULL;			// List of active queries
pthread_mutex_t mutex4= PTHREAD_MUTEX_INITIALIZER;	// For Query list

/*********************\
|*  Local variables  *|
 \*********************/

// Temporary buffer used for writing, logging, etc.
static char tmp_buf[8000];

// Local functions
gboolean callback_query_timeout(gpointer data);

/*****************************************\
|* Functions that handle the Query list  *|
\*****************************************/

// Create a Query descriptor and put it in qlist; starts the timer
Query *new_Query(const char *filename, uint32_t seq, gboolean is_ipv6) {
	// Add the additional parameters to store the IPv6/IPv4 and port numbers of the
	// Query sender, to later forward the Hit message to this address.
	Query *pt;
	assert((filename!=NULL));
	pt = (Query *) malloc(sizeof(Query));
	strncpy(pt->name, filename, sizeof(pt->name)-1);
	pt->seq = seq;
	pt->thread = NULL;
    // ** TASK 2.1 **
	// pt->state = ...
	pt->is_ipv6 = is_ipv6;
	pt->timer_id= 0;
	// Define the query Id - for logging purposes
	sprintf(pt->qname, "'%s'(%d)%s", filename, seq, is_ipv6 ? "v6" : "v4");
	// ** TASK 1.2 **
	// Store the IPv6, IPv4 and port number
	// ...
	pt->self_ = pt;
	pthread_mutex_lock(&mutex4);
	qlist = g_list_append(qlist, pt);
	pthread_mutex_unlock(&mutex4);
	return pt;
}

// Update Hit information in qlist
void update_HIT_info(Query *q, unsigned long long flen, u_int fhash) {
	assert(q != NULL);
	// Update them only if you added these values to the structure
	// ...
}

// Search for Query descriptor in qlist
Query *locate_qid_in_Querylist(const char *filename, uint32_t seq) {
	assert(filename != NULL);
	GList *list;
	pthread_mutex_lock(&mutex4);
	for (list = qlist; list != NULL; list = g_list_next(list)) {
		if (((Query *) list->data)->seq == seq)
			if (!strcmp(filename, ((Query *) list->data)->name)) {
				pthread_mutex_unlock(&mutex4);
				return (Query *) list->data;
			}
	}
	pthread_mutex_unlock(&mutex4);
	return NULL;
}

// Search for Query descriptor in qlist
Query *locate_qid_in_Querylist_by_IPv(const char *filename, uint32_t seq, gboolean is_ipv6) {
	assert(filename != NULL);
	GList *list;
	pthread_mutex_lock(&mutex4);
	for (list = qlist; list != NULL; list = g_list_next(list)) {
		Query *q = (Query *) list->data;
		if (q->seq == seq)
			if (!strcmp(filename, q->name))
				if (q->is_ipv6 == is_ipv6)
					if (q->self_ == q) {
						pthread_mutex_unlock(&mutex4);
						return (Query *) list->data;
					}
	}
	pthread_mutex_unlock(&mutex4);
	return NULL;
}

// Free qlist descriptor and all pending memory
void free_query_in_Querylist(Query *q, gboolean called_from_GUI) {
	assert(q != NULL);
	if ((q == NULL) || (q->self_ != q))
		return;

    // ** TASK 2.2 **
	// Stop timer if it is active
	// ...

	q->self_ = NULL;	// It is being freed

	// Delete from GUI
	GUI_del_Query(q->name, q->seq, q->is_ipv6, called_from_GUI);

	pthread_mutex_lock(&mutex4);
	qlist = g_list_remove(qlist, q);
	pthread_mutex_unlock(&mutex4);

	// Stop thread if it is active
	// ...

	free(q);
}

// Abort and free all active queries
void delete_Querylist(gboolean called_from_GUI) {
	while (qlist != NULL) {
		Query *pt = (Query *) qlist->data;
		free_query_in_Querylist(pt, called_from_GUI);
		if ((qlist!=NULL) && (pt == qlist->data)) {
			fprintf(stderr, "Internal error in del_query_list()\n");
			break;
		}
	}
}

/*******************************************************\
|* Functions to control the state of the application   *|
 \*******************************************************/

// Extracts the directory name from a full path name
static const char *get_trunc_filename(const char *FileName) {
	char *pt = strrchr(FileName, (int) '/');
	if (pt != NULL)
		return pt + 1;
	else
		return FileName;
}

// Start timer
void start_query_timer(Query *q, long int timeout) {
	if ((q == NULL) || (q->self_ != q))
		return;
	q->timer_id = g_timeout_add(timeout, callback_query_timeout, q);
#ifdef DEBUG
	fprintf(stderr, "%s started timer %u : %ld ms\n", q->qname, q->timer_id, timeout);
#endif
}

// Stop timer
void stop_query_timer(Query *q) {
	if ((q == NULL) || (q->self_ != q))
		return;
	if (q->timer_id > 0) {
		g_source_remove(q->timer_id);
#ifdef DEBUG
		fprintf(stderr, "%s stopped timer %u\n", q->qname, q->timer_id);
#endif
		q->timer_id = 0;
	}
}

// Callback that handles query timeouts
gboolean callback_query_timeout(gpointer data) {
	Query *q = (Query *) data;
	if ((q == NULL) || (q->self_ != q)) {
		fprintf(stderr, "Error in callback_query_timeout - invalid data\n");
		return FALSE; // stop timer
	}

#ifdef DEBUG
	g_print("Callback query_timeout (%s) - timer %u\n", q->qname, q->timer_id);
#endif

	// ** TASK 2.2 ** // ** TASK 2.3 ** // ** TASK 3.2 **
	// Put here what you should do when the timer ends!

	// It should depend on the query state

	// If you are running a jitter timer before transmitting the Query, you should send the Query
	//  using send_multicast. The jitter value should be set to
	//			long int jitter_time = (long) floor(1.0 * random() / RAND_MAX * QUERY_JITTER);

	// If you are waiting for a Hit, cancel the pending Query

	// If you are waiting for a connection, cancel the pending Query

	// Otherwise, it is a mistake

	q->timer_id= 0;
	return FALSE;  // stop timer

	return FALSE;  // stop timer
}

// Handle the reception of a Query packet
void handle_Query(char *buf, int buflen, gboolean is_ipv6,
		struct in6_addr *ipv6, struct in_addr *ipv4, u_short port) {
	uint32_t seq;
	const char *fname;

	assert((buf != NULL) && ((is_ipv6&&(ipv6!=NULL)) || (!is_ipv6&&(ipv4!=NULL))));
	if (!read_query_message(buf, buflen, &seq, &fname)) {
		Log("Invalid Query packet\n");
		return;
	}

	assert(fname != NULL);
	if (strcmp(fname, get_trunc_filename(fname))) {
		Log("ERROR: The Query must not include the pathname - use 'get_trunc_filename'\n");
		return;
	}

	char tmp_ip[100];	// temporary buffer to store a string with the sender's IP address
	if (is_ipv6)
		strcpy(tmp_ip, addr_ipv6(ipv6));
	else
		strcpy(tmp_ip, addr_ipv4(ipv4));
	if (is_local_ip(tmp_ip) && (port == portUDPq)) {
		// Ignore local loopback
		return;
	}

	sprintf(tmp_buf, "Received Query '%s'(%d) from [%s]:%hu\n", fname, seq,
			(is_ipv6 ? addr_ipv6(ipv6) : addr_ipv4(ipv4)), port);
	Log(tmp_buf);


	Log("handle_Query not implemented yet\n");

	// ** TASK 1.2 ** - initial partial implementation for QUERY from IPv6
	// ** TASK 2.2 + 2.4 ** - updated version with timer support

	// Check if it is a new query (equal name+seq+is_ipv6) searching the Query list - ignore the query if it is an old one
	// ... Use the Querylist

	// Check if the query has appeared in the !is_ipv6 domain - if it has, ignore it because someone else send it before.
	// ... use the Querylist

	// If it is new, create a new Query struct and store it in the Querylist
	//		Query *pt= new_Query(fname, seq, is_ipv6, ...);
	// Add the query to the GUI
	//
	// NOTE: If you think that this qlist stuff is just too much for you, you can use the graphical functions in gui.h (but they are less powerful)

	// Store the query information in the GUI table list
	GUI_add_Query(fname, seq, is_ipv6, strdup(tmp_ip), port);

	// (TASK x) Start by forwarding the Query message to the other domain (i.e. !is_ipv6) using:
	// ... send_multicast(buf, buflen, !is_ipv6);
	//
	// At TASK x, if you have time, replace with starting a jitter timer, which will send the Query later!
	//	This helps when there are more than one gateway is connecting two multicast groups!
	// ... start_query_timer(pt, jitter_time);
}

// Handle the reception of an Hit packet
void handle_Hit(char *buf, int buflen, struct in6_addr *ip, u_short port,
		gboolean is_ipv6) {
	char hbuf[MSG_BUFFER_SIZE];		// sending HIT buffer
	int hlen;						// sending HIT message length
	uint32_t seq;
	char fname[CNAME_LENGTH+1];
	unsigned long long flen;
	uint32_t fhash;
	unsigned short sTCP_port;
	struct in6_addr srvIP6;
	struct in_addr srvIP4;

	assert ((buf!=NULL) && (ip!=NULL));

	if (!read_hit_message(buf, buflen, &seq, fname, &flen, &fhash,
			&sTCP_port, is_ipv6, &srvIP6, &srvIP4)) {
		Log("Received invalid Hit packet\n");
		return;
	}
	// String with the IP address of the server fileexchange
	char ipstr[40];	// Maximum size for an IPv6 address is "abcd:" x 8 = 40
	sprintf(ipstr, "%s", is_ipv6 ? addr_ipv6(&srvIP6) : addr_ipv4(&srvIP4));

	sprintf(tmp_buf,
			"Received Hit '%s' (IP= %s; port= %hu; Len=%llu; Hash=%hu) from [%s]:%hu\n",
			fname, ipstr, sTCP_port, flen, fhash, addr_ipv6(ip), port);
	Log(tmp_buf);

	Log("handle_Hit not implemented yet\n");

	// ** TASK 1.2 **
	// Test here if this HIT matches one of the pending Query contents in the list
	// If not, ignore the Hit received
	// Query *q = ...

	// Add HIT to GUI list
	sprintf(tmp_buf, "%s-%hu", addr_ipv6(ip), sTCP_port);
	GUI_add_hit_to_Query(fname, seq, !is_ipv6, tmp_buf);

	if (TRUE/* HIT comes from IPv4 fileexchange == QUERY came from IPv6 */) {
		/***********************************************/
		/**** HIT received from an IPv4 fileexchange ***/
		/***********************************************/

		// ** TASK 1.3 ** - Initial implementation
		// ** TASK 2.3 ** - updated version with timer support

		// Get the client information from your Query entry.
		// You may also get the client's information from the graphical table calling
		// gboolean GUI_get_Query_details(const char *filename, uint16_t seq, gboolean is_ipv6, const char **str_ip, unsigned int *port, const char **hits);
		//	   str_ip has the IP address and port has the port number of the client.

		// HIT was received from IPv4 - convert the serverIP to the IPV6 dual stack equivalent
		// ...

		// Write the HIT packet contents to a buffer
		// Use the function write_hit_message(hbuf, &hlen, seq, fname, fhash, flen, sTCP_port, ...) to prepare the Hit message
		//     in buffer hbuf, where the missing parameter contains the fileexchange's IP addresses is a string with the server's IPv6 address converted from IPv4
		// ...


		// Send the HIT packet to the client fileexchange
		//    use the function send_M6reply(... , ... , hbuf, hlen) to send the Hit

		// In order to avoid not seeing the Query in the graphical table, you must wait for a timeout to clear the GUI entry
		// Otherwise, you can clear it from the GUI table here using:
		//		GUI_del_Query(fname, seq, !is_ipv6, FALSE);
		//
		// Start the timer and wait for timeout to clear the GUI entry
		// ...

#ifdef DEBUG
//		sprintf(tmp_buf,
//				"Sent Hit '%s' (IP= %s, port= %hu; Len=%llu; Hash=%u) to %s:%hu\n",
//				fname, /*server_ipv6*/, portTCP, flen, fhash, addr_ipv6(&???), q->???);
//		Log(tmp_buf);
#endif
		return;

	}

	/***********************************************/
	/**** HIT received from an IPv6 fileexchange ***/
	/***********************************************/
	// ** PHASE 3 - TASK 3.1 + 3.2 ** - updated version with timer support

	// Test if an HIT was previously received and a thread is already active
	// ...

	// ... update_HIT_info(q, fhash, flen);	// Update Query/Hit table in the GUI

	// Add the proxy information to the GUI, just to inform that you are expecting a connection using
	// ... GUI_add_Proxy(filename, seq);
	// where filename and seq are stored in the Query structure
	// The proxy thread will only be created when a new TCP connection is received


	// Update the additional fields added to the Query state struct and update the timer
	// ...


	// Prepare a new HIT message with the proxy information and send it to the client.
	// Use  write_hit_message(...) and
	//		send_message4(&client_ipv4_address, client_port, HIT_buffer, HIT_buflen)
	// where client_ipv4_address and client_port should be in the Query structure
	// Remember that you need to send a valid IPv4 address to the IPv4 fileexchange, identifying
	//  the gateway TCP port, and later, when you receive the TCP communication, this
	//  thread will be associated with the Query.
	// The local IPv4 address is available through "sock.h" in variable local_ipv4
	// ...

	// Start the timer, to wait for up to QUERY_TIMEOUT microseconds for a connection
	// ...

#ifdef DEBUG
//	sprintf(tmp_buf,
//			"Sent Hit '%s' (IP= %s, port= %hu; Len=%llu; Hash=%u) to %s:%hu\n",
//			fname, addr_ipv4(&local_ipv4), portTCP, flen, fhash, inet_ntoa(q->...), q->...);
//	Log(tmp_buf);
#endif
}

// Handle the reception of a new connection on a server socket
// Return TRUE if it should accept more connections; FALSE otherwise
gboolean handle_new_connection(int sock, struct sockaddr_in6 *cli_addr) {
	assert(sock >= 0);
	assert(cli_addr != NULL);

	thread_state *state = new_thread_state(sock, cli_addr);

	// Run a new proxy thread
	int err = pthread_create(&state->tid, NULL, proxy_function, (void *) state);
	if (err) {
		fprintf(stderr, "Error starting thread: return code %d\n", err);
	}

	return TRUE;
}

// Callback button 'Stop': stops the selected TCP transmission and associated proxy
void on_buttonStop_clicked(GtkButton *button, gpointer user_data) {
	GtkTreeIter iter;
	const char *fname;
	uint16_t seq;
	int Tsock;

	if (GUI_get_selected_Proxy(&fname, &seq, &Tsock, &iter)) {
#ifdef DEBUG
		g_print("Proxy with socket %d will be stopped\n", Tsock);
#endif
	} else {
		Log("No proxy selected\n");
		return;
	}
	if (Tsock <= 0) {
		Log("Invalid TCP socket in the selected line\n");
		return;
	}

	gtk_list_store_remove(main_window->listProxies, &iter);

	// Stop proxy thread
	sprintf(tmp_buf, "Stopping Query/thread to %s:%d\n", fname, seq);
	Log(tmp_buf);
	stop_thread(fname, seq);
}

// Closes everything
void close_all(gboolean called_from_GUI) {
	// Close all sockets
	close_sockTCP();
	close_sockUDP();
	// Threads stop when the sockets are closed - memory is freed at the end of a thread

	// Stop all remaining active queries
	delete_Querylist(called_from_GUI);
}

// Button that starts and stops the application
void on_togglebuttonActive_toggled(GtkToggleButton *togglebutton,
		gpointer user_data) {

	if (gtk_toggle_button_get_active(togglebutton)) {

		// *** Starts the server ***
		const gchar *addr4_str, *addr6_str;
		int n4, n6;

		n4 = get_PortIPv4Multicast();
		n6 = get_PortIPv6Multicast();
		if ((n4 < 0) || (n6 < 0)) {
			Log("Invalid multicast port number\n");
			gtk_toggle_button_set_active(togglebutton, FALSE); // Turns button off
			return;
		}
		port_MCast4 = (unsigned short) n4;
		port_MCast6 = (unsigned short) n6;

		addr6_str = get_IPv6Multicast(NULL);
		addr4_str = get_IPv4Multicast(NULL);
		if (!addr6_str && !addr4_str) {
			gtk_toggle_button_set_active(togglebutton, FALSE); // Turns button off
			return;
		}
		if (!init_sockets(port_MCast4, addr4_str, port_MCast6, addr6_str)) {
			Log("Failed configuration of server\n");
			gtk_toggle_button_set_active(togglebutton, FALSE); // Turns button off
			return;
		}
		//set_PortTCP(port_TCP);
		set_PID(getpid());
		//
		block_entrys(TRUE);
		active = TRUE;
		Log("gateway active\n");

	} else {

		// *** Stops the server ***
		active = FALSE;
		close_all(TRUE);
		block_entrys(FALSE);
		set_PID(0);
		Log("gateway stopped\n");
	}

}

// Callback function that handles the end of the closing of the main window
gboolean on_window1_delete_event(GtkWidget * widget, GdkEvent * event,
		gpointer user_data) {
	gtk_main_quit();	// Close Gtk main cycle
	return FALSE;// Must always return FALSE; otherwise the window is not closed.
}

