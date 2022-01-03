CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -O3



COMMON_HEADER = helpers.h

SERVER_HEADERS = client.h topic.h structures.h $(COMMON_HEADER)
SUBSCRIBER_HEADERS = $(COMMON_HEADER)

SERVER_SOURCES = server.c $(SERVER_HEADERS:.h=.c)
SUBSCRIBER_SOURCES = subscriber.c $(SUBSCRIBER_HEADERS:.h=.c)

SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
SUBSCRIBER_OBJECTS = $(SUBSCRIBER_SOURCES:.c=.o)

SERVER_FILES = $(SERVER_SOURCES) $(SERVER_HEADERS)
SUBSCRIBER_FILES = $(SUBSCRIBER_SOURCES) $(SUBSCRIBER_HEADERS)

SERVER_EXEC = server
SUBSCRIBER_EXEC = subscriber



SERVER_PORT = 7772
SERVER_IP = 127.0.0.1
CLIENT_ID = defaultID



.PHONY: default build clean run_server run_subscriber run_udp

default: build

build: $(SERVER_EXEC) $(SUBSCRIBER_EXEC)

%.o: %.c $(SERVER_HEADERS) $(SUBSCRIBER_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_EXEC): $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) $(SERVER_OBJECTS) -o $@ -lm

$(SUBSCRIBER_EXEC): $(SUBSCRIBER_OBJECTS)
	$(CC) $(CFLAGS) $(SUBSCRIBER_OBJECTS) -o $@

run_server: default
	./$(SERVER_EXEC) $(SERVER_PORT)

run_subscriber: default
	./$(SUBSCRIBER_EXEC) $(CLIENT_ID) $(SERVER_IP) $(SERVER_PORT)

run_udp: default
	python3 udp_client.py $(SERVER_IP) $(SERVER_PORT) --mode manual

clean:
	rm -f $(SERVER_EXEC) $(SUBSCRIBER_EXEC)
	rm -f $(SERVER_OBJECTS)
	rm -f $(SUBSCRIBER_OBJECTS)