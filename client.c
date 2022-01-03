#include "helpers.h"
#include "client.h"

Elem *clients_root = NULL;
Elem *waiting_root = NULL;

Client *new_client(char *id, int socket, struct sockaddr_in addr) {
	Client *client = malloc(sizeof(Client));

	DIE(!client, "[server] ERROR: Couldn't alloc new client\n");

	memset(client->id, 0, ID_LEN + 1);
	memcpy(client->id, id, ID_LEN);
	client->socket = socket;
	memcpy(&(client->addr), &addr, SOCKADDR_SIZE);
	client->message_queue = new_queue();

	return client;
}

void add_client(Client *client, Elem **root) {
	// create the node
	Elem *elem = new_elem((void *) client);

	// insert it at the beginning
	elem->next = (*root);
	// set it as root
	(*root) = elem;
}

Client *get_client(char *id, int socket, Elem *root) {
	// find a client by their ID or socket, NULL if nonexistent

	// empty list
	if (!root) {
		return NULL;
	}

	// linear search
	Elem *curr_elem = root;

	while (curr_elem) {
		Client *client = (Client *) (curr_elem->data);

		if ((socket != -1 && client->socket == socket) ||
			(socket == -1 && id && !strcmp(client->id, id))) {
			// client found by socket or by id
			return client;
		}

		// advance
		curr_elem = curr_elem->next;
	}

	// if it reached here, no client has been found;
	return NULL;
}

Client *extract_client(char *id, int socket, Elem **root) {
	// search by ID or socket
	Elem *prev_elem = NULL;
	Elem *curr_elem = (*root);

	while (curr_elem) {
		Client *curr_client = (Client *) (curr_elem->data);

		if ((socket != -1 && curr_client->socket == socket) ||
			(socket == -1 && id && !strcmp(curr_client->id, id))) {
			// client found
			break;
		}

		// advance
		prev_elem = curr_elem;
		curr_elem = curr_elem->next;
	}

	Client *to_return = NULL;

	if (curr_elem) {
		to_return = (Client *) (curr_elem->data);

		if (prev_elem) {
			// not first elem
			prev_elem->next = curr_elem->next;
		} else {
			// first elem
			(*root) = curr_elem->next;
		}

		free(curr_elem);
	}

	return to_return;
}

void client_login(Client *client, int socket) {
	if (client->socket != -1) {
		// already ONLINE
		return;
	}

	client->socket = socket; // now ONLINE

	// check messages received while OFFLINE (from SF subscriptions)
	Elem *curr_elem = NULL;
	while ((curr_elem = dequeue(client->message_queue))) {
		char parsed_message[MESS_LEN + 1] = {0};
		char *curr_message = (char *) (curr_elem);
		memcpy(parsed_message, curr_message, MESS_LEN);

		// send
		int result = send(client->socket, parsed_message, MESS_LEN, 0);
		DIE(result < 0, "[server] ERROR: Error while sending stored message "
						"to SF client '%s', on socket '%d'\n",
						client->id,
						client->socket);
		GOOD("[server] INFO: Sent stored message to SF client '%s', on socket "
			"'%d'\n", client->id, client->socket);

		free(curr_message);
	}
}

void client_logout(Client *client) {
	client->socket = -1; // now OFFLINE
}

void delete_client(Elem *elem) {
	if (!elem) return;

	// recursively free the successors
	delete_client(elem->next);

	// delete this one
	Client *client = (Client *) (elem->data);

	delete_queue(client->message_queue);
	free(client);
	free(elem);
}

void remove_clients() {
	delete_client(clients_root);
}
