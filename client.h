#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "helpers.h"
#include "structures.h"

typedef struct client_struct {
	char id[ID_LEN + 1];
	int socket; // -1 means the client is offline
	struct sockaddr_in addr;

	Queue *message_queue;
} Client;

extern Elem *waiting_root;
extern Elem *clients_root;

Client *new_client(char *id, int socket, struct sockaddr_in addr);
void add_client(Client *client, Elem **root);

Client *get_client(char *id, int socket, Elem *root);
Client *extract_client(char *id, int socket, Elem **root);

void client_login(Client *client, int socket);
void client_logout(Client *client);

void delete_client(Elem *elem);
void remove_clients(); // operates on clients_root

#endif