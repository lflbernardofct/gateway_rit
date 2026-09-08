/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MEEC/MERSIM - FCT NOVA  2026/2027
 *
 * gui_g3.c
 *
 * Functions that handle graphical user interface interaction
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "gui.h"
#include "callbacks.h"

// Set here the glade file name
#define GLADE_FILE "gateway.glade"

pthread_mutex_t mutex1= PTHREAD_MUTEX_INITIALIZER;	// For logging

/** Initialization function */
gboolean
init_app (WindowElements *win)
{
        GtkBuilder              *builder;
        GError                  *err=NULL;

        /* use GtkBuilder to build our interface from the XML file */
        builder = gtk_builder_new ();
        if (gtk_builder_add_from_file (builder, GLADE_FILE, &err) == 0)
        {
                error_message (err->message);
                g_error_free (err);
                return FALSE;
        }
        /* get the widgets which will be referenced in callbacks */
        win->window = GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "window1"));
        win->entryIPv6 = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryIPv6"));
        win->entryIPv4 = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryIPv4"));
        win->entryIPMv6 = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryM_IPv6"));
        win->entryM6Port = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryMPort_v6"));
        win->entryIPMv4 = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryM_IPv4"));
        win->entryM4Port = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryMPort_v4"));
        win->entryTCPPort = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryTCPport"));
        win->entryPID = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryPID"));
        win->checkbuttonSlow = GTK_TOGGLE_BUTTON(gtk_builder_get_object (builder,
                											 "checkbuttonSlow"));
        win->treeQueries = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                             "treeview1"));
        win->listQueries = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                             "liststore_queries"));
        win->treeProxies = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                             "treeview2"));
        win->listProxies = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                             "liststore_proxies"));
        win->textView = GTK_TEXT_VIEW (gtk_builder_get_object (builder,
                                                                "textview1"));

        /* connect signals, passing our TutorialTextEditor struct as user data */
        gtk_builder_connect_signals (builder, win);
        /* free memory used by GtkBuilder object */
        g_object_unref (G_OBJECT (builder));

        return TRUE;
}


/** Logs the message str to the textview and command line */
void Log (const gchar * str)
{
  GtkTextBuffer *textbuf;
  GtkTextIter tend;

  // Adds text to the command line
  g_print("%s", str);
  // Use Mutex to avoid uncoherent writing to the log window
  pthread_mutex_lock(&mutex1);
  textbuf = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (main_window->textView));
  gtk_text_buffer_get_iter_at_offset (textbuf, &tend, -1);	// Gets reference to the last position
  // Adds text to the textview
  gtk_text_buffer_insert (textbuf, &tend, g_strdup (str), strlen (str));
  pthread_mutex_unlock(&mutex1);
}

/** Sets the content of Local IPv6 */
void set_LocalIPv6(const char *addr) {
	assert(addr != NULL);
	gtk_entry_set_text(main_window->entryIPv6, addr);
}

/** Sets the content of Local IPv4 */
void set_LocalIPv4(const char *addr) {
	assert(addr != NULL);
	gtk_entry_set_text(main_window->entryIPv4, addr);
}


/** Get the content of entryIPv6, validating if it is a valid IPv6 address.
 *   addrv6 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv6Multicast(struct in6_addr *addrv6) {
	struct in_addr addrv4;
	struct in6_addr addrv6_aux, *addrv6_pt;
	const gchar *textIP;

	textIP= gtk_entry_get_text(main_window->entryIPMv6);
	if (textIP == NULL)
		return NULL;
	addrv6_pt= (addrv6==NULL) ? &addrv6_aux : addrv6;

	if (inet_pton(AF_INET, textIP, &addrv4)) {
		Log("Invalid IPv6 address: sockets IPv6 cannot use IPv4 multicast addresses\n");
		return NULL;
	} else if (inet_pton(AF_INET6, textIP, addrv6_pt)) {
		if ((textIP[0]=='f' || textIP[0]=='F') && (textIP[1]=='f' || textIP[1]=='F'))
			return textIP;
		else {
			Log("Invalid IPv6 address: it is not multicast (must start with FF01:: - FF1E::)\n");
			return NULL;
		}
	} else {
		Log("Invalid IPv6 address\n");
		return NULL;
	}
}

/** Sets the content of entryIPv6 */
void set_IPv6Multicast(const char *addr) {
	assert(addr != NULL);
	gtk_entry_set_text(main_window->entryIPMv6, addr);
}

/** Get the content of entryIPv4, validating if it is a valid IPv4 address.
 *   addrv4 is returned with the address in numerical format.
 *   Returns the address name, or NULL if it is not valid */
const gchar *get_IPv4Multicast(struct in_addr *addrv4) {
	struct in_addr addrv4_aux, *addrv4_pt;
	const gchar *textIP;

	textIP= gtk_entry_get_text(main_window->entryIPMv4);
	if (textIP == NULL)
		return NULL;
	addrv4_pt= (addrv4==NULL) ? &addrv4_aux : addrv4;

	if (inet_pton(AF_INET, textIP, addrv4_pt)) {
		if (!IN_MULTICAST(ntohl(addrv4_pt->s_addr))) {
			Log("Invalid IPv4 address: not multicast\n");
			return NULL;
		} else
			return textIP;
	} else {
		Log("Invalid IPv4 address\n");
		return NULL;
	}
}

/** Sets the content of entryIPv4 */
void set_IPv4Multicast(const char *addr) {
	assert(addr != NULL);
	gtk_entry_set_text(main_window->entryIPMv4, addr);
}

/** Get the slow checkbuttin state */
gboolean get_checkbutton_Slow_state() {
	return gtk_toggle_button_get_active (main_window->checkbuttonSlow);
}

// Translates a string into its numerical value
static int get_number_from_text(const gchar *text) {
  int n= 0;
  char *pt;

  if ((text != NULL) && (strlen(text)>0)) {
    n= strtol(text, &pt, 10);
    return ((pt==NULL) || (*pt)) ? -1 : n;
  } else
    return -1;
}

/** Validates a port number */
static short get_PortNumber(GtkEntry *entryText) {
	const char *textPort= gtk_entry_get_text(entryText);
	int port= get_number_from_text(textPort);
	if (port>(powl(2,15)-1))
		port= -1;
	return (short)port;
}

/** Get the IPv6 Multicast port number. Return the port number, or -1 if error */
short get_PortIPv6Multicast() {
	return get_PortNumber(main_window->entryM6Port);
}

/** Set the IPv6 Multicast port number entry */
void set_PortIPv6Multicast(unsigned short port) {
	char buf[10];
	sprintf(buf, "%hu", port);
	gtk_entry_set_text(main_window->entryM6Port, buf);
}

/** Get the IPv4 Multicast port number. Return the port number, or -1 if error */
short get_PortIPv4Multicast() {
	return get_PortNumber(main_window->entryM4Port);
}

/** Set the IPv4 Multicast port number entry */
void set_PortIPv4Multicast(unsigned short port) {
	char buf[10];
	sprintf(buf, "%hu", port);
	gtk_entry_set_text(main_window->entryM4Port, buf);
}

/** Gets the TCP port number. Return the port number, or -1 if error */
short get_PortTCP() {
	return get_PortNumber(main_window->entryTCPPort);
}

/** Sets the TCP port number entry */
void set_PortTCP(unsigned short port) {
	char buf[10];
	sprintf(buf, "%hu", port);
	gtk_entry_set_text(main_window->entryTCPPort, buf);
}

/** Gets the PID number. Return -1 if error */
int get_PID() {
	return get_number_from_text(gtk_entry_get_text(main_window->entryPID));
}

/** Sets the PID number entry */
void set_PID(int pid) {
	char buf[10];
	sprintf(buf, "%d", pid);
	gtk_entry_set_text(main_window->entryPID, buf);
}

/** Blocks or unblocks the configuration GtkEntry boxes in the GUI */
void block_entrys(gboolean block)
{
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryTCPPort), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryIPMv6), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryM6Port), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryIPMv4), !block);
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryM4Port), !block);
  }



/*************************************************\
|*  Functions that manage the querylist TreeView  *|
 \*************************************************/

/** Search for a query in the queries tree list */
gboolean GUI_locate_Query(const char *filename, uint16_t seq, gboolean is_ipv6,
		GtkTreeIter *iter) {
	assert(filename != NULL);
	assert(iter != NULL);
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listQueries);
	gboolean valid;

	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first(list_store, iter);

	while (valid) {
		// Walk through the list, reading each row
		gchar *str_fname;
		u_int str_seq;
		gboolean str_is_ipv6;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get(list_store, iter, 3, &str_fname, 4, &str_seq, 5,
				&str_is_ipv6, -1);

		// Do something with the data
#ifdef DEBUG
		g_print("QUERY for: Filename='%s' Seq=%d %s\n", str_fname, str_seq,
				str_is_ipv6 ? "IPv6" : "IPv4");
#endif
		if (!strcmp(str_fname, filename) && (str_seq == seq)
				&& (str_is_ipv6 == is_ipv6)) {
			g_free(str_fname);
			return TRUE;
		}
		valid = gtk_tree_model_iter_next(list_store, iter);
		g_free(str_fname);
	}
	return FALSE;
}

/** Search for a query in the queries tree list */
gboolean GUI_locate_Query_by_filename(const char *filename, GtkTreeIter *iter) {
	assert(filename != NULL);
	assert(iter != NULL);
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listQueries);
	gboolean valid;

	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first(list_store, iter);

	while (valid) {
		// Walk through the list, reading each row
		gchar *str_fname;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get(list_store, iter, 3, &str_fname, -1);

		// Do something with the data
#ifdef DEBUG
		g_print("QUERY for: Filename='%s'\n", str_fname);
#endif
		if (!strcmp(str_fname, filename)) {
			g_free(str_fname);
			return TRUE;
		}
		valid = gtk_tree_model_iter_next(list_store, iter);
		g_free(str_fname);
	}
	return FALSE;
}


/** Return ip, port and hits of a Query */
gboolean GUI_get_Query_details(const char *filename, uint16_t seq, gboolean is_ipv6,
		const char **str_ip, unsigned int *port, const char **hits) {
	if ((str_ip == NULL) || (port == NULL) || (hits == NULL)) {
		Log("ERROR: Invalid parameters in GUI_get_Query_details()\n");
		return FALSE;
	}

	GtkTreeIter iter;
	gboolean ok= FALSE;

	/* get GTK thread lock */
	gdk_threads_enter ();

	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listQueries);

	if (GUI_locate_Query(filename, seq, is_ipv6, &iter)) {
		gtk_tree_model_get(list_store, &iter, 2, str_ip, 1, port, 0, hits, -1);
		ok= TRUE;
	}
	/* release GTK thread lock */
	gdk_threads_leave ();
	return ok;	// Not found
}

/** Return the hits of a Query */
gboolean GUI_get_Query_hits(const char *filename, uint16_t seq, gboolean is_ipv6,
		const char **hits) {
	if (hits == NULL) {
		Log("ERROR: Invalid parameters in GUI_get_Query_hits()\n");
		return FALSE;
	}

	GtkTreeIter iter;
	gboolean ok= FALSE;

	/* get GTK thread lock */
	gdk_threads_enter ();
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listQueries);

	if (GUI_locate_Query(filename, seq, is_ipv6, &iter)) {
		gtk_tree_model_get(list_store, &iter, 0, hits, -1);
		ok= TRUE;
	}
	/* release GTK thread lock */
	gdk_threads_leave ();
	return ok;	// Not found
}

/** Adds a query to the query table */
gboolean GUI_add_Query(const char *filename, uint16_t seq, gboolean is_ipv6, const char *ip, u_int port) {
	assert(filename != NULL);
	assert(ip != NULL);

	GtkTreeIter iter;

	/* get GTK thread lock */
	gdk_threads_enter ();
	if (GUI_locate_Query(filename, seq, is_ipv6, &iter)) {
		Log("Replacing query\n");
	} else {
		// new file
		gtk_list_store_append(main_window->listQueries, &iter);
	}
	gtk_list_store_set(main_window->listQueries, &iter, 0, g_strdup(""), 1, port,
			2, ip, 3, g_strdup(filename), 4, (uint)seq, 5, is_ipv6, -1);
	/* release GTK thread lock */
	gdk_threads_leave ();
	return TRUE;
}

/** Adds an hit information to the query table */
gboolean GUI_add_hit_to_Query(const char *filename, uint16_t seq, gboolean is_ipv6,
		const char *hit) {
	assert(filename != NULL);
	assert(hit != NULL);
	GtkTreeIter iter;
	gboolean ok= FALSE;

	/* get GTK thread lock */
	gdk_threads_enter ();
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listQueries);

	if (GUI_locate_Query(filename, seq, is_ipv6, &iter)) {
		char *str_hit, *new_hit;
		gtk_tree_model_get(list_store, &iter, 0, &str_hit, -1);
		if (strlen(str_hit) > 0)
			new_hit = g_strdup_printf("%s %s", str_hit, hit);
		else
			new_hit = g_strdup(hit);
		gtk_list_store_set(main_window->listQueries, &iter, 0, new_hit, -1);
		// g_free(str_hit);
		ok= TRUE;
	}
	/* release GTK thread lock */
	gdk_threads_leave ();
	return ok;
}

/** Deletes a query from the query table */
gboolean GUI_del_Query(const char *filename, uint16_t seq, gboolean is_ipv6, gboolean called_from_GUI) {
	assert(filename != NULL);
	gboolean ok= FALSE;

	GtkTreeIter iter;
	if (!called_from_GUI) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listQueries);
	assert(list_store != NULL);

	if (GUI_locate_Query(filename, seq, is_ipv6, &iter)) {
		gtk_list_store_remove(GTK_LIST_STORE(list_store), &iter);
		ok= TRUE;
	}
	if (!called_from_GUI) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


/***************************************************\
|*  Functions that manage the proxy list TreeView  *|
 \**************************************************/

/** Search for a query in the proxy treeview list */
gboolean GUI_locate_Proxy(const char *filename, uint16_t seq, GtkTreeIter *iter, gboolean lockGtkThread) {
	assert(filename != NULL);
	assert(iter != NULL);
	gboolean valid;

	if (lockGtkThread) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listProxies);


	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first(list_store, iter);

#ifdef DEBUG
	g_print("GUI_locate_Proxy query for: Filename='%s' Seq=%u\n\tLocated:", filename, seq);
#endif
	while (valid) {
		// Walk through the list, reading each row
		gchar *str_fname;
		u_int str_seq;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get(list_store, iter, 0, &str_fname, 7, &str_seq, -1);

		// Do something with the data
#ifdef DEBUG
		g_print(" '%s'-%u", str_fname, str_seq);
#endif
		if (!strcmp(str_fname, filename) && (str_seq == seq)) {
			g_free(str_fname);
#ifdef DEBUG
			g_print("\n");
#endif
			if (lockGtkThread) {
				/* release GTK thread lock */
				gdk_threads_leave ();
			}
			return TRUE;
		}
		valid = gtk_tree_model_iter_next(list_store, iter);
		g_free(str_fname);
	}
#ifdef DEBUG
	g_print("\n");
#endif
	if (lockGtkThread) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return FALSE;
}

/** Search for a TCP port in the proxy treeview list */
gboolean GUI_locate_Proxy_by_TCPsocket(int TCPsock, GtkTreeIter *iter) {
	assert(iter != NULL);
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listProxies);
	gboolean valid;

	// Get the first iter in the list
	valid = gtk_tree_model_get_iter_first(list_store, iter);

#ifdef DEBUG
	g_print("GUI_locate_Proxy_by_TCPsocket: QUERY for TCP socket='%u'\n\tFound:", TCPsock);
#endif
	while (valid) {
		// Walk through the list, reading each row
		guint str_sock;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get(list_store, iter, 1, &str_sock, -1);

		// Do something with the data
#ifdef DEBUG
		g_print(" '%u'", str_sock);
#endif
		if (str_sock == TCPsock) {
#ifdef DEBUG
			g_print("\n");
#endif
			return TRUE;
		}
		valid = gtk_tree_model_iter_next(list_store, iter);
	}
#ifdef DEBUG
	g_print("\n");
#endif
	return FALSE;
}

/** Add a proxy to the proxy treeview list */
gboolean GUI_add_Proxy(const char *filename, uint16_t seq) {
	assert(filename != NULL);

	GtkTreeIter iter;

	/* get GTK thread lock */
	gdk_threads_enter ();

	if (GUI_locate_Proxy(filename, seq, &iter, FALSE)) {
		Log("Replacing proxy\n");
	} else {
		// new file
		gtk_list_store_append(main_window->listProxies, &iter);
	}
	gtk_list_store_set(main_window->listProxies, &iter, 0, g_strdup(filename), 1, 0,
			2, g_strdup(""), 3, 0, 4, g_strdup(""), 5, 0, 6, 0, 7, seq, -1);

	/* release GTK thread lock */
	gdk_threads_leave ();
	return TRUE;
}

/** Updates the IP and port of the client socket (connection accepted from the client) of a proxy */
gboolean GUI_update_cli_details_Proxy(const char *filename, uint16_t seq, int TCPsock, const char *ip, u_int port) {
	GtkTreeIter iter;
	gboolean ok= FALSE;

	/* get GTK thread lock */
	gdk_threads_enter ();
	if (GUI_locate_Proxy(filename, seq, &iter,FALSE)) {
		gtk_list_store_set(main_window->listProxies, &iter, 1, TCPsock, 2, g_strdup(ip), 3, port, -1);
		ok= TRUE;
	}
	/* release GTK thread lock */
	gdk_threads_leave ();
	return ok;
}

/** Updates the IP and port of the server socket (connection created to the server) of a proxy */
gboolean GUI_update_serv_details_Proxy(int TCPsock, const char *ip, u_int port) {
	GtkTreeIter iter;
	gboolean ok= FALSE;

	/* get GTK thread lock */
	gdk_threads_enter ();
	if (GUI_locate_Proxy_by_TCPsocket(TCPsock, &iter)) {
		gtk_list_store_set(main_window->listProxies, &iter, 4, g_strdup(ip), 5, port, -1);
		ok= TRUE;
	}
	/* release GTK thread lock */
	gdk_threads_leave ();
	return ok;
}

/** Updates the transfer percentage of a proxy connection */
gboolean GUI_update_transf_Proxy(u_int TCPsock, u_int transf) {
	GtkTreeIter iter;
	gboolean ok= FALSE;

	/* get GTK thread lock */
	gdk_threads_enter ();
	if (GUI_locate_Proxy_by_TCPsocket(TCPsock, &iter)) {
		gtk_list_store_set(main_window->listProxies, &iter, 6, transf, -1);
		ok= TRUE;
	}
	/* release GTK thread lock */
	gdk_threads_leave ();
	return ok;
}

/** Get selected proxy data; returns FALSE if none is selected; returns TRUE and iter pointing to the line */
gboolean GUI_get_selected_Proxy(const char **filename, uint16_t *seq, int *sock, GtkTreeIter *iter) {
	GtkTreeSelection *selection;
	GtkTreeModel	*model;
	gboolean ok= FALSE;
	u_int useq;

	assert(iter != NULL);
	assert(filename != NULL);
	assert(seq!=NULL);
	assert(sock!=NULL);

	selection= gtk_tree_view_get_selection(main_window->treeProxies);
	if ((selection != NULL) && gtk_tree_selection_get_selected(selection, &model, iter)) {
		gtk_tree_model_get (model, iter, 0, filename, 1, sock, 7, &useq, -1);
		*seq= (u_int16_t)useq;
		ok= TRUE;
	}

	return ok;
}

/** Deletes a proxy from the proxy treeview list; if filename==NULL searches only for TCP port */
gboolean GUI_del_Proxy(const char *filename, uint16_t seq, int sock, gboolean called_from_GUI) {
	GtkTreeModel *list_store = GTK_TREE_MODEL(main_window->listProxies);
	assert(list_store != NULL);

	GtkTreeIter iter;
	gboolean ok= FALSE;

	if (!called_from_GUI) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	if (((filename != NULL) && GUI_locate_Proxy(filename, seq, &iter,FALSE)) ||
			((filename == NULL) && (GUI_locate_Proxy_by_TCPsocket(sock, &iter)))) {
		gtk_list_store_remove(GTK_LIST_STORE(list_store), &iter);
		ok= TRUE;
	}
	if (!called_from_GUI) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return ok;
}


/***************************\
|*   Auxiliary functions   *|
\***************************/

/** Creates a window with an error message and outputs it to the command line */
void
error_message (const gchar *message)
{
        GtkWidget               *dialog;

        /* log to terminal window */
        g_warning ("%s", message);

        /* create an error message dialog and display modally to the user */
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "%s", message);

        gtk_window_set_title (GTK_WINDOW (dialog), "Error!");
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}



/***********************\
|*   Event handlers    *|
\***********************/

/** Handles 'Clear' button - clears textMemo */
void on_buttonClear_clicked (GtkButton * button, gpointer user_data)
{
  GtkTextBuffer *textbuf;
  GtkTextIter tbegin, tend;

  textbuf = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (main_window->textView));
  gtk_text_buffer_get_iter_at_offset (textbuf, &tbegin, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &tend, -1);
  gtk_text_buffer_delete (textbuf, &tbegin, &tend);
}


