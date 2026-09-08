/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MEEC/MERSIM - FCT NOVA  2026/2027
 *
 * gui.h
 *
 * Header file of functions that handle graphical user interface interaction
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#ifndef _INCL_GUI_H
#define _INCL_GUI_H

#include <gtk/gtk.h>
#include <netinet/in.h>

/* store the widgets which may need to be accessed in a typedef struct */
typedef struct
{
        GtkWidget               *window;
        GtkEntry				*entryIPv6;
        GtkEntry				*entryIPv4;
        GtkEntry				*entryIPMv6;
        GtkEntry				*entryM6Port;
        GtkEntry				*entryIPMv4;
        GtkEntry				*entryM4Port;
        GtkEntry				*entryTCPPort;
        GtkEntry				*entryPID;
        GtkToggleButton			*checkbuttonSlow;
        GtkTreeView				*treeQueries;
        GtkListStore			*listQueries;
        GtkTreeView				*treeProxies;
        GtkListStore			*listProxies;
        GtkTextView				*textView;
} WindowElements;

// Global pointer to the main window elements
extern WindowElements *main_window;

// Initialization function
gboolean init_app (WindowElements *window);

// Logs the message str to the textview and command line
void Log (const gchar * str);



/************************************************\
|*   Functions that read and update text boxes  *|
\************************************************/

/** Sets the content of Local IPv6 */
void set_LocalIPv6(const char *addr);
/** Sets the content of Local IPv4 */
void set_LocalIPv4(const char *addr);

/** Get the content of entryIPv6, validating if it is a valid IPv6 address.
 *   addrv6 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv6Multicast(struct in6_addr *addrv6);
/** Sets the content of entryIPv6 */
void set_IPv6Multicast(const char *addr);

/** Get the content of entryIPv4, validating if it is a valid IPv4 address.
 *   addrv4 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv4Multicast(struct in_addr *addrv4);
/** Sets the content of entryIPv4 */
void set_IPv4Multicast(const char *addr);

/** Get the IPv6 Multicast port number. Return the port number, or -1 if error */
short get_PortIPv6Multicast();
/** Set the IPv6 Multicast port number entry */
void set_PortIPv6Multicast(unsigned short port);
/** Get the IPv4 Multicast port number. Return the port number, or -1 if error */
short get_PortIPv4Multicast();
/** Set the IPv4 Multicast port number entry */
void set_PortIPv4Multicast(unsigned short port);

/** Get the slow checkbuttin state */
gboolean get_checkbutton_Slow_state();

/** Gets the TCP port number. Return the port number, or -1 if error */
short get_PortTCP();
/** Sets the TCP port number entry */
void set_PortTCP(unsigned short port);

/** Gets the PID number. Return -1 if error */
int get_PID();
/** Sets the PID number entry */
void set_PID(int pid);

// Blocks or unblocks the configuration GtkEntry boxes in the GUI
void block_entrys(gboolean block);


/*************************************************\
|*  Functions that manage the querylist TreeView  *|
 \*************************************************/

// Search for a query in the queries treeview list by filename, seq and is_ipv6
gboolean GUI_locate_Query(const char *filename, uint16_t seq, gboolean is_ipv6,
		GtkTreeIter *iter);
// Search for a query in the queries treeview list by filename
gboolean GUI_locate_Query_by_filename(const char *filename,
		GtkTreeIter *iter);
// Return ip, port and hits of a Query
gboolean GUI_get_Query_details(const char *filename, uint16_t seq, gboolean is_ipv6,
		const char **str_ip, unsigned int *port, const char **hits);
// Return the hits of a Query
gboolean GUI_get_Query_hits(const char *filename, uint16_t seq, gboolean is_ipv6, const char **hits);
// Adds a query to the query table
gboolean GUI_add_Query(const char *filename, uint16_t seq, gboolean is_ipv6, const char *ip, u_int port);
// Adds an hit information to the query table
gboolean GUI_add_hit_to_Query(const char *filename, uint16_t seq, gboolean is_ipv6, const char *hit);
// Deletes a query from the query table
gboolean GUI_del_Query(const char *filename, uint16_t seq, gboolean is_ipv6, gboolean called_from_GUI);


/***************************************************\
|*  Functions that manage the proxy list TreeView  *|
 \**************************************************/

// Search for a query in the proxy treeview list
gboolean GUI_locate_Proxy(const char *filename, uint16_t seq, GtkTreeIter *iter, gboolean lockGtkThread);
// Search for a TCP socket in the proxy treeview list
gboolean GUI_locate_Proxy_by_TCPsock(int sock, GtkTreeIter *iter);
// Add a proxy to the proxy treeview list
gboolean GUI_add_Proxy(const char *filename, uint16_t seq);
// Updates the IP and port of the client socket (connection accepted from the client) of a proxy
gboolean GUI_update_cli_details_Proxy(const char *filename, uint16_t seq, int TCPsock, const char *ip, u_int port);
// Updates the IP and port of the server socket (connection created to the server) of a proxy
gboolean GUI_update_serv_details_Proxy(int TCPsock, const char *ip, u_int port);
// Updates the transfer percentage of a proxy connection
gboolean GUI_update_transf_Proxy(u_int TCPsock, u_int transf);
// Get selected proxy data; returns FALSE if none is selected
gboolean GUI_get_selected_Proxy(const char **filename, uint16_t *seq, int *sock, GtkTreeIter *iter);
// Deletes a proxy from the proxy treeview list
gboolean GUI_del_Proxy(const char *filename, uint16_t seq, int sock, gboolean called_from_GUI);


/***************************\
|*   Auxiliary functions   *|
\***************************/

/** Creates a window with an error message and outputs it to the command line */
void error_message (const gchar *message);

/***********************\
|*   Event handlers    *|
\***********************/
// Event handlers in gui_g3.c

// Handles 'Clear' button - clears textMemo
void on_buttonClear_clicked (GtkButton *button, gpointer user_data);

// External event handlers in callbacks.c
void on_togglebutton1_toggled (GtkToggleButton *togglebutton, gpointer user_data);
// External event handlers in proxy_channel.c
void on_buttonStop_clicked (GtkButton *button, gpointer user_data);

#endif



