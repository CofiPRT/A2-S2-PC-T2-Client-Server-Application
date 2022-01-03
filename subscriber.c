#include "helpers.h"

int validate_message(char *buffer) {
	// TRUE if it's valid
	char command[COMMAND_LEN] = {0};
	char topic[TOPIC_LEN + 1] = {0};
	int sf_flag;

	int args = sscanf(buffer, "%s %s %d", command, topic, &sf_flag);
	DIE(args == 0, "[client] ERROR: Invalid command\n");

	if (!strcmp(command, "subscribe")) {
		DIED(args != 3, "[client] ERROR: Invalid number of arguments for "
						"command 'subscribe'\n"
						"\tExpected:\n\t\tsubscribe TOPIC SF\n"
						"\tReceived:\n\t\t%s\n", buffer);

		if (DIED_RESULT) return FALSE;
	} else if (!strcmp(command, "unsubscribe")) {
		DIED(args != 2, "[client] ERROR: Invalid number of arguments for "
						"command 'unsubscribe'\n"
						"\tExpected:\n\t\tunsubscribe TOPIC\n"
						"\tReceived:\n\t\t%s\n", buffer);

		if (DIED_RESULT) return FALSE;
	} else if (!strcmp(command, "exit")) {
		DIED(args != 1, "[client] ERROR: Invalid number of arguments for "
						"command 'exit'\n"
						"\tExpected:\n\t\texit\n"
						"\tReceived:\n\t\t%s\n", buffer);
		if (DIED_RESULT) return FALSE;
	} else {
		DIED(TRUE, "[client] ERROR: Invalid command\n"
					"\tExpected one of the following:\n"
					"\t\tsubscribe TOPIC SF\n"
					"\t\tunsubscribe TOPIC\n"
					"\t\texit\n"
					"\tReceived:\n\t\t%s\n", buffer);
		return FALSE;
	}

	return TRUE;
}

int main(int argc, char *argv[]) {
	DIE(argc < 4, "[client] ERROR: Usage: %s ID SERVER_IP PORT\n", argv[0]);

	// open socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "[client] ERROR: Could not open socket\n");
	GOOD("[client] INFO: Successfully opened socket '%d'\n", sockfd);

	// verify PORT
	short port = atoi(argv[3]);
	DIE(port <= 0, "[client] ERROR: Invalid port or error in 'atoi'. "
					"Called with port number '%s'", argv[3]);
	GOOD("[client] INFO: Working on port '%d'\n", port);

	// deactive Neagle's algorithm
	if (!USE_NEAGLE) {
		int flag = 1;

		int result = setsockopt(sockfd, SOL_TCP, TCP_NODELAY,
								(void *) &flag, sizeof(int));
		DIE(result < 0, "[client] ERROR: Could not deactivate Neagle's "
						"algorithm\n");
		GOOD("[client] INFO: Successfully deactivated Neagle's "
						"algorithm\n");
	}

	// fill in the sockaddr struct
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	int result = inet_aton(argv[2], &server_addr.sin_addr);
	DIE(result == 0, "[client] ERROR: Error in function 'inet_aton', "
						"called with '%s'\n", argv[2]);
	GOOD("[client] INFO: Successfully filled in server_addr details\n");

	// connect to server
	struct sockaddr *cast_addr = (struct sockaddr *) &server_addr;

	result = connect(sockfd, cast_addr, SOCKADDR_SIZE);
	DIE(result < 0, "[client] ERROR: Could not connect on "
					"socket '%d'\n", sockfd);
	GOOD("[client] INFO: Successfully connected to socket '%d'\n", sockfd);

	// send connect message to server, containing our ID
	char buffer[BUFLEN] = {0};
	sprintf(buffer, "connect %s", argv[1]);

	result = send(sockfd, buffer, strlen(buffer), 0);
	DIE(result < 0, "[client] ERROR: Could not send connect message "
					"on socket '%d'\n", sockfd);
	GOOD("[client] INFO: Successfully sent 'connect' message to server\n");

	// initialise the descriptor sets
	fd_set cli_read_fds;
	fd_set cli_tmp_fds;
	FD_ZERO(&cli_read_fds);
	FD_ZERO(&cli_tmp_fds);

	// add the server socket and STDIN
	FD_SET(sockfd, &cli_read_fds);
	FD_SET(0, &cli_read_fds);

	int fdmax = sockfd;

	while (TRUE) {
		// clone the set
		cli_tmp_fds = cli_read_fds;

		result = select(fdmax + 1, &cli_tmp_fds, NULL, NULL, NULL);
		DIE(result < 0, "[client] ERROR: Could not select message\n");
		GOOD("[client] INFO: Successfully selected message\n");

		memset(buffer, 0, BUFLEN);

		if (FD_ISSET(0, &cli_tmp_fds)) {
			// input from STDIN

			result = read(0, buffer, BUFLEN);
			DIE(result < 0, "[client] ERROR: Could not read from STDIN\n");
			GOOD("[client] INFO: Successfully read from STDIN\n");

			// command validity
			if (VALIDATE_COMMANDS && !validate_message(buffer)) {
				// not valid
				continue;
			}

			// send to server
			result = send(sockfd, buffer, strlen(buffer), 0);
			DIE(result < 0, "[client] ERROR: Could not send message "
							"on socket '%d'\n", sockfd);
			GOOD("[client] INFO: Successfully sent message on "
				"socket '%d'\n", sockfd);

			if (strncmp(buffer, "exit", 4) == 0) {
				// exit command
				break;
			}

			char command[COMMAND_LEN] = {0};
			char topic[TOPIC_LEN] = {0};
			sscanf(buffer, "%s %s", command, topic);

			// log
			printf("%sd %s\n", command, topic);
		} else {
			// message rom server
			char message[MESS_LEN] = {0};
			
			result = recv(sockfd, message, MESS_LEN, 0);
			DIE(result < 0, "[client] ERROR: Error while receiving message "
							"on socket '%d'\n", sockfd);
			GOOD("[client] INFO: Successfully received message on "
				"socket '%d'\n", sockfd);

			if (result == 0) {
				// server shut down
				break;
			}

			// log message
			printf("%s\n", message);
		}
	}

	// signal shutdown to server
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	GOOD("[client] INFO: Successfully shut down the client\n");
}