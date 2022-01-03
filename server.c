#include "helpers.h"
#include "structures.h"
#include "client.h"
#include "topic.h"

// stuff we work with

// main sockets
int sockfd_tcp = -1;
int sockfd_udp = -1;
int fd_max = -1;

// clients
fd_set read_fds; // descriptor list

void init_sockets(char *port_char) {
	// open TCP socket
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "[server] ERROR: Could not open TCP socket\n");
	GOOD("[server] INFO: Opened TCP socket on '%d'\n", sockfd_tcp);

	// open UDP socket
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "[server] ERROR: Could not open UDP socket\n");
	GOOD("[server] INFO: Opened UDP socket on '%d'\n", sockfd_udp);

	// verify PORT
	short port = atoi(port_char);
	DIE(port <= 0, "[server] ERROR: Invalid port or error in 'atoi'. "
					"Called with port number '%s'", port_char);
	GOOD("[server] INFO: Working on port '%d'\n", port);

	// fill in the sockaddr struct
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	struct sockaddr *cast_addr = (struct sockaddr *) &server_addr;

	// bind TCP socket
	int result = bind(sockfd_tcp, cast_addr, SOCKADDR_SIZE);
	DIE(result < 0, "[server] ERROR: Could not bind TCP socket\n");
	GOOD("[server] INFO: Bound TCP socket\n");

	// bind UDP socket
	result = bind(sockfd_udp, cast_addr, SOCKADDR_SIZE);
	DIE(result < 0, "[server] ERROR: Could not bind UDP socket\n");
	GOOD("[server] INFO: Bound UDP socket\n");

	// set the TCP socket as LISTENING socket
	result = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(result < 0, "[server] ERROR: Could not listen on socket '%d'\n",
					sockfd_tcp);
	GOOD("[server] INFO: Set TCP on listening mode\n");
}

void accept_client() {
	struct sockaddr_in client_addr;

	// create a new socket for this client
	struct sockaddr *cast_addr = (struct sockaddr *) &client_addr;
	socklen_t length = sizeof(client_addr);

	int client_fd = accept(sockfd_tcp, cast_addr, &length);
	DIE(client_fd < 0, "[server] ERROR: Could not accept the new connection\n");
	GOOD("[server] INFO: Accepted new connection on socket '%d'\n", client_fd);

	// deactive Neagle's algorithm
	if (!USE_NEAGLE) {
		int flag = 1;

		int result = setsockopt(client_fd, SOL_TCP, TCP_NODELAY,
								(void *) &flag, sizeof(int));
		DIE(result < 0, "[client] ERROR: Could not deactivate Neagle's "
						"algorithm\n");
		GOOD("[client] INFO: Successfully deactivated Neagle's "
						"algorithm\n");
	}

	// add to set
	FD_SET(client_fd, &read_fds);
	fd_max = max(fd_max, client_fd);

	// add to waiting (for ID) list
	Client *client = new_client("", client_fd, client_addr);
	add_client(client, &waiting_root);
}

void manage_client(int client_fd) {
	char buffer[BUFLEN] = {0};

	// store the message received on this socket
	int result = recv(client_fd, buffer, sizeof(buffer), 0);
	DIE(result < 0, "[server] ERROR: Error while receiving message from "
					"socket '%d'\n", client_fd);
	GOOD("[server] INFO: Received message on socket '%d':\n%s\n", client_fd,
																	buffer);

	char command[COMMAND_LEN] = {0};
	char topic[TOPIC_LEN + 1] = {0};
	int sf_flag;

	int args = sscanf(buffer, "%s %s %d", command, topic, &sf_flag);
	DIE(args == 0, "[server] ERROR: Invalid message from "
					"socket '%d'\n", client_fd);
	GOOD("[server] INFO: Parsed message from socket '%d'\n", client_fd);

	if (!strcmp(command, "connect")) {
		DIE(args != 2, "[server] ERROR: Invalid number of arguments for "
						"command 'connect' "
						"(Expected 'connect ID', "
						"received '%s'\n", buffer);
		GOOD("[server] INFO: Recognized command 'connect'\n");

		// this client is connecting, 'topic' actually contains their ID
		Client *client = get_client(topic, -1, clients_root);

		if (!client) {
			// logging in for the first time
			// search by socket
			client = extract_client(NULL, client_fd, &waiting_root);
			DIE(!client, "[server] ERROR: Client '%s' not yet "
							"accepted\n", topic);
			GOOD("[server] INFO: Extracted client with socket '%d' "
				"from the waiting list\n", client_fd);

			// copy ID
			strncpy(client->id, topic, ID_LEN);
			add_client(client, &clients_root);
		}

		client_login(client, client_fd);

		// log
		printf("New client %s connected from %s:%u.\n",
				client->id,
				inet_ntoa(client->addr.sin_addr),
				ntohs(client->addr.sin_port));

	} else if (!strcmp(command, "subscribe")) {
		DIE(args != 3, "[server] ERROR: Invalid number of arguments for "
						"command 'subscribe'\n"
						"\tExpected:\n\t\tsubscribe TOPIC SF\n"
						"\tReceived:\n\t\t%s\n", buffer);
		GOOD("[server] INFO: Recognized command 'subscribe'\n");

		// the client requests a subscription
		Client *client = get_client(NULL, client_fd, clients_root);
		DIE(!client, "[server] ERROR: Client nonexistent on "
						"socket '%d'\n", client_fd);
		GOOD("[server] INFO: Found client with socket '%d'\n", client_fd);

		Topic *topic_t = get_topic(topic);

		subscribe(client, topic_t, sf_flag);

		GOOD("[server] INFO: Successfully subscribed client\n");
	} else if (!strcmp(command, "unsubscribe")) {
		DIE(args != 2, "[server] ERROR: Invalid number of arguments for "
						"command 'unsubscribe'\n"
						"\tExpected:\n\t\tunsubscribe TOPIC\n"
						"\tReceived:\n\t\t%s\n", buffer);
		GOOD("[server] INFO: Recognized command 'unsubscribe'\n");

		// the client request a subscription cancellation
		Client *client = get_client(NULL, client_fd, clients_root);
		DIE(!client, "[server] ERROR: Client nonexistent on "
						"socket '%d'\n", client_fd);
		GOOD("[server] INFO: Found client with socket '%d'\n", client_fd);

		Topic *topic_t = get_topic(topic);

		unsubscribe(client, topic_t);

		GOOD("[server] INFO: Successfully unsubscribed client\n");
	} else if (!strcmp(command, "exit")) {
		DIE(args != 1, "[server] ERROR: Invalid number of arguments for "
						"command 'exit'\n"
						"\tExpected:\n\t\texit\n"
						"\tReceived:\n\t\t%s\n", buffer);
		GOOD("[server] INFO: Recognized command 'exit'\n");

		// the client is disconnecting
		Client *client = get_client(NULL, client_fd, clients_root);
		DIE(!client, "[server] ERROR: Client nonexistent on "
						"socket '%d'\n", client_fd);
		GOOD("[server] INFO: Found client with socket '%d'\n", client_fd);

		FD_CLR(client_fd, &read_fds);
		client_logout(client);

		// log
		printf("Client %s disconnected.\n", client->id);
	} else {
		DIE(TRUE, "[server] ERROR: Invalid command\n"
					"\tExpected one of the following:\n"
					"\t\tsubscribe TOPIC SF\n"
					"\t\tunsubscribe TOPIC\n"
					"\t\texit\n"
					"\tReceived:\n\t\t%s\n", buffer);
	}
}

char *parse_message(char *buffer, struct sockaddr_in addr) {
	char *parsed_message = calloc(MESS_LEN + 1, 1);

	char topic[TOPIC_LEN + 1] = {0};
	char data_type = 0;
	char type[TYPE_LEN + 1] = {0};
	char payload[PAYLOAD_LEN + 1] = {0};

	// extract from buffer
	memcpy(topic, buffer, TOPIC_LEN);
	memcpy(&data_type, buffer + TOPIC_LEN, 1);

	// extract payload
	if (data_type == 0) {
		// parse as INT

		char sign = 0;
		memcpy(&sign, buffer + TOPIC_LEN + 1, 1);

		uint32_t number = 0;
		memcpy(&number, buffer + TOPIC_LEN + 2, 4);

		number = ntohl(number);

		if (sign) {
			// signed
			sprintf(payload, "-%u", number);
		} else {
			// unsigned
			sprintf(payload, "%u", number);
		}

		sprintf(type, "INT");
	} else if (data_type == 1) {
		// parse as SHORT_REAL

		uint16_t number = 0;
		memcpy(&number, buffer + TOPIC_LEN + 1, 2);

		number = ntohs(number);

		float div = ((float) (number)) / 100;

		sprintf(payload, "%.2f", div);

		sprintf(type, "SHORT_REAL");
	} else if (data_type == 2) {
		// parse as FLOAT

		char sign = 0;
		memcpy(&sign, buffer + TOPIC_LEN + 1, 1);

		uint32_t number = 0;
		memcpy(&number, buffer + TOPIC_LEN + 2, 4);

		number = ntohl(number);

		uint8_t exp = 0;
		memcpy(&exp, buffer + TOPIC_LEN + 6, 1);

		double div = ((float) (number)) / powf(10, exp);

		if (sign) {
			// signed
			sprintf(payload, "-%lf", div);
		} else {
			// unsigned
			sprintf(payload, "%lf", div);
		}

		sprintf(type, "FLOAT");
	} else if (data_type == 3) {
		// parse as STRING

		memcpy(payload, buffer + TOPIC_LEN + 1, PAYLOAD_LEN);

		sprintf(type, "STRING");
	}

	// final sprintf
	sprintf(parsed_message, "%s:%u - %s - %s - %s",
			inet_ntoa(addr.sin_addr),
			ntohs(addr.sin_port),
			topic,
			type,
			payload);

	return parsed_message;
}

void manage_udp() {
	char buffer[UDP_LEN] = {0};

	// store the message received on the UDP socket
	struct sockaddr_in addr;
	memset(&addr, 0, SOCKADDR_SIZE);

	struct sockaddr *cast_addr = (struct sockaddr *) &addr;
	socklen_t length = SOCKADDR_SIZE;

	int result = recvfrom(sockfd_udp, buffer, sizeof(buffer), 0,
							cast_addr, &length);
	DIE(result < 0, "[server] ERROR: Error while receiving message from "
					"the UDP socket\n");
	GOOD("[server] INFO: Received message on the UDP socket\n");

	// parse the message
	char *parsed_message = parse_message(buffer, addr);
	GOOD("[server] INFO: Parsed message from UDP "
		"socket:\n%s\n", parsed_message);

	// extract the topic
	char topic[TOPIC_LEN + 1] = {0};

	memcpy(topic, buffer, TOPIC_LEN);
	GOOD("[server] INFO: Parsed topic from UDP socket:\n%s\n", topic);

	Topic *topic_t = get_topic(topic);

	// notify normal clients
	Elem *curr_elem = topic_t->clients;

	while (curr_elem) {
		Client *client = (Client *) (curr_elem->data);

		if (client->socket != -1) {
			// normally subbed client, forward only if ONLINE
			result = send(client->socket, parsed_message, MESS_LEN, 0);
			DIE(result < 0, "[server] ERROR: Error while sending message to "
				"client '%s', on socket '%d'\n", client->id, client->socket);
			GOOD("[server] INFO: Successfully sent message to client '%s' on "
				"socket '%d'\n", client->id, client->socket);
		}

		// advance
		curr_elem = curr_elem->next;
	}

	// notify SF clients
	curr_elem = topic_t->sf_clients;

	while (curr_elem) {
		Client *client = (Client *) (curr_elem->data);

		if (client->socket != -1) {
			// client is ONLINE, everything good
			result = send(client->socket, parsed_message, MESS_LEN, 0);
			DIE(result < 0, "[server] ERROR: Error while sending message to SF "
				"client '%s', on socket '%d'\n", client->id, client->socket);
			GOOD("[server] INFO: Successfully sent message to SF client '%s' "
				"on socket '%d'\n", client->id, client->socket);
		} else {
			// client is OFFLINE, store this message in their queue
			char *aux_message = calloc(MESS_LEN + 1, 1);
			memcpy(aux_message, parsed_message, MESS_LEN);

			enqueue(client->message_queue, (void *) aux_message);

			GOOD("[server] INFO: Stored message for SF client '%s' for "
				"future transmission\n", client->id);
		}

		// advance
		curr_elem = curr_elem->next;
	}

	free(parsed_message);
	GOOD("[server] INFO: Successfully managed UDP message\n");
}

void manage_server() {
	fd_set tmp_fds; // auxiliary descriptor list

	// initialise descriptor sets
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// add the TCP and UDP sockets (and STDIN)
	FD_SET(sockfd_tcp, &read_fds);
	FD_SET(sockfd_udp, &read_fds);
	FD_SET(0, &read_fds);

	// highest numbered file descriptor (used as argument in 'select')
	fd_max = max(sockfd_tcp, sockfd_udp);

	// run indefinitely
	while (TRUE) {
		// clone the set
		tmp_fds = read_fds;

		GOOD("[server] INFO: Waiting to select message\n");

		// select file descriptor ready for reading
		int result = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(result < 0, "[server] ERROR: Could not select message\n");
		GOOD("[server] INFO: Successfully selected message\n");

		if (FD_ISSET(0, &tmp_fds)) {
			// input from STDIN

			// read the message
			char buffer[BUFLEN] = {0};
			
			result = read(0, buffer, BUFLEN);
			DIE(result < 0, "[server] ERROR: Could not read from STDIN\n");
			GOOD("[server] INFO: Successfully read from STDIN\n");

			if (strncmp(buffer, "exit", 4) == 0) {
				// exit command received, stop the server
				break;
			}
		}

		// find selected socket
		for (int i = 1; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					// new incoming TCP client, accept
					GOOD("[server] INFO: Accepting client\n");
					accept_client();
				} else if (i == sockfd_udp) {
					// UDP message received
					GOOD("[server] INFO: Managing UDP message\n");
					manage_udp();
				} else {
					// client message received
					GOOD("[server] INFO: Managing client message\n");
					manage_client(i);
				}
			}
		}
	}
}

void stop_server() {
	// close connections
	for (int i = 1; i < fd_max; i++) {
		if (FD_ISSET(i, &read_fds) && i != sockfd_tcp && i != sockfd_udp) {
			// shutdown
			shutdown(i, SHUT_RDWR);
			close(i);

			FD_CLR(i, &read_fds);
		}
	}

	// free alloc'd data
	remove_clients();
	remove_topics();

	// close sockets
	close(sockfd_tcp);
	close(sockfd_udp);
}

int main(int argc, char *argv[]) {
	DIE(argc < 2, "[server] ERROR: Usage: %s PORT\n", argv[0]);

	// initialise sockets
	init_sockets(argv[1]);

	// do stuff
	manage_server();

	// stop it
	stop_server();

	GOOD("[server] INFO: Successfully shut down server\n");
}