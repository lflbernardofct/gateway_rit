#*****************************************************************************\
#* Redes Integradas de Telecomunicacoes
#* MEEC/MERSIM - FCT NOVA  2026/2027
#*
#* Makefile
#*
#* @author  Luis Bernardo
#*****************************************************************************/
GNOME_INCLUDES= `pkg-config --cflags --libs gtk+-3.0` -lm 
CFLAGS= -Wall -Wno-deprecated-declarations -g -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -pthread
# CFLAGS= -Wall -Wno-deprecated-declarations -g -DDEBUG -pthread
# CFLAGS= -Wall -Wno-deprecated-declarations -g -pthread
# CFLAGS= -Wall -Wno-deprecated-declarations -O3 -pthread

APP_NAME= gateway
APP_MODULES= sock.o gui_g3.o callbacks.o callbacks_socket.o proxy_thread.o

all: $(APP_NAME)
	
clean: 
	rm -f $(APP_NAME) *.o

$(APP_NAME): main.c $(APP_MODULES) gui.h sock.h callbacks.h proxy_thread.h
	gcc $(CFLAGS) -o $(APP_NAME) main.c $(APP_MODULES) $(GNOME_INCLUDES) -export-dynamic

sock.o: sock.c sock.h gui.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) sock.c -export-dynamic

gui_g3.o: gui_g3.c gui.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) gui_g3.c -export-dynamic
	
callbacks.o: callbacks.c callbacks.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) callbacks.c -export-dynamic

callbacks_socket.o: callbacks_socket.c callbacks_socket.h callbacks.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) callbacks_socket.c -export-dynamic

proxy_thread.o: proxy_thread.c proxy_thread.h callbacks_socket.h callbacks.h sock.h
	gcc $(CFLAGS) -c $(GNOME_INCLUDES) proxy_thread.c -export-dynamic

