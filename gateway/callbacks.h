/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MEEC/MERSIM - FCT NOVA  2026/2027
 *
 * callbacks.h
 *
 * Header file of functions that handle main application logic for UDP communication,
 *    controlling query forwarding
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#include <gtk/gtk.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif


// Program constants
#define CNAME_LENGTH			80		// Maximum filename
#define MSG_BUFFER_SIZE			64000	// Message buffer size
#define QUERY_JITTER			100		/* Jitter time for Query retransmission - 100 mseconds */
#define QUERY_TIMEOUT			10000	/* Query timeout - 10 seconds */
#define HIT_CONNECTION_TIMEOUT	10000	/* Wait for connection timeout - 10 seconds */


struct Hit;
struct Query;



#ifndef INCL_CALLBACKS_
#define INCL_CALLBACKS_

#include "gui.h"
#include "proxy_thread.h"


// ** TASK 2.1 **
// typedef enum { ... list of states ... } QueryState;
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// Query information
typedef struct Query {
	char			qname[CNAME_LENGTH+41];	// Query id string
	uint32_t		seq;		// Sequence number
    char			name[CNAME_LENGTH+1]; 	// Name looked up

    guint 			timer_id; 	// Timer event associated

  // Sender information
    gboolean		is_ipv6;	// Sender domain
    // ** TASK 1.2 **
    // ... add ipv6/ipv4 addresses and port number ...

  // State information
    // ** TASK 2.1 **
    // QueryState 		state;		// Query state
    thread_state   *thread;

    // Add any additional field you want!
    // modify the constructor function (new_Query)!

    struct Query   *self_;
} Query;



/**********************\
|*  Global variables  *|
\**********************/

extern gboolean active; // TRUE if server is active

// Main window
extern WindowElements *main_window;


/*****************************************\
|* Functions that handle the Query list  *|
\*****************************************/

// Create a Query descriptor and put it in qlist - add parameters to store the QUERY's IP and port number
// ** TASK 1.2 **
Query *new_Query(const char *filename, uint32_t seq, gboolean is_ipv6 /*, ...*/);
// Search for Query descriptor in qlist
Query *locate_qid_in_Querylist(const char *filename, uint32_t seq);
Query *locate_qid_in_Querylist_by_IPv(const char *filename, uint32_t seq, gboolean is_ipv6);
// Update Hit information in qlist
void update_HIT_info(Query *q, unsigned long long flen, u_int fhash);
// Free qlist descriptor and all pending memory
void free_query_in_Querylist(Query *ppt, gboolean called_from_GUI);
// Abort and free all active queries
void delete_Querylist(gboolean called_from_GUI);

/*******************************************************\
|* Functions to control the state of the application   *|
\*******************************************************/

// Start timer
void start_query_timer(Query *q, long int timeout);
// Stop timer
void stop_query_timer(Query *q);
// Handle the reception of a Query packet
void handle_Query(char *buf, int buflen, gboolean is_ipv6, struct in6_addr *ipv6, struct in_addr *ipv4, u_short port);
// Handle the reception of an Hit packet
void handle_Hit(char *buf, int buflen, struct in6_addr *ip, u_short port, gboolean is_ipv6);
// Handle the reception of a new connection on a server socket
gboolean handle_new_connection(int sock, struct sockaddr_in6 *cli_addr);

// Closes everything
void close_all(gboolean called_from_GU);

// Button that starts and stops the application
void on_togglebuttonActive_toggled(GtkToggleButton *togglebutton, gpointer user_data);

// Callback button 'Stop': stops the selected TCP transmission and associated proxy
void on_buttonStop_clicked(GtkButton *button, gpointer user_data);

// Callback function that handles the end of the closing of the main window
gboolean on_window1_delete_event (GtkWidget * widget,
		GdkEvent * event, gpointer user_data);

#endif
