#include "helpers.h"
#include "topic.h"
#include "client.h"

Elem *topics_root = NULL;

Topic *new_topic(char *name) {
	Topic *topic = malloc(sizeof(Topic));

	DIE(!topic, "[server] ERROR: Couldn't alloc new topic\n");

	memset(topic->name, 0, TOPIC_LEN + 1);
	memcpy(topic->name, name, TOPIC_LEN);
	topic->clients = NULL;
	topic->sf_clients = NULL;

	return topic;
}

Topic *get_topic(char *name) {
	// find a topic by name, or create one if nonexistent
	
	// empty list
	if (!topics_root) {
		// create the topic
		Topic *topic = new_topic(name);
		// set it as root
		topics_root = new_elem((void *) topic);
		// return it
		return topic;
	}

	// linear search
	Elem *prev_elem = NULL;
	Elem *curr_elem = topics_root;

	while (curr_elem) {
		Topic *topic = (Topic *) (curr_elem->data);

		if (!strcmp(topic->name, name)) {
			// topic found by name
			return topic;
		}

		// advance
		prev_elem = curr_elem;
		curr_elem = curr_elem->next;
	}

	// if it reached here, no topic has been found; create it
	Topic *topic = new_topic(name);
	// add it to the list
	prev_elem->next = new_elem((void *) topic);
	// return it
	return topic;
}

void subscribe(Client *client, Topic *topic, int sf_flag) {
	// check if already subscribed
	Client *temp_client = get_client(NULL, client->socket, topic->clients);

	if (temp_client) {
		// already subscribed normally

		if (sf_flag && CHANGEABLE_SUB) {
			// change subscription type
			unsubscribe(client, topic);
			subscribe(client, topic, 1);
		}

		return;
	}

	// check if already subscribed as SF
	temp_client = get_client(NULL, client->socket, topic->sf_clients);

	if (temp_client) {
		// already subscribed as SF

		if (!sf_flag && CHANGEABLE_SUB) {
			// change subscription type
			unsubscribe(client, topic);
			subscribe(client, topic, 0);
		}

		return;
	}

	// if it reached here, the client is not subscribed to this topic
	if (sf_flag) {
		// subscribe as SF
		add_client(client, &(topic->sf_clients));
	} else {
		// subscribe normally
		add_client(client, &(topic->clients));
	}
}

void unsubscribe(Client *client, Topic *topic) {
	// check if subscribed normally
	Client *client_t = extract_client(NULL, client->socket, &(topic->clients));

	if (client_t) {
		// no need to check if subscribed as SF
		return;
	}

	// check if subscribed as SF
	extract_client(NULL, client->socket, &(topic->sf_clients));
}

void delete_topic(Elem *elem) {
	if (!elem) return;

	// recursively free the successors
	delete_topic(elem->next);

	// delete this one
	Topic *topic = (Topic *) (elem->data);

	free(topic);
	free(elem);
}

void remove_topics() {
	delete_topic(topics_root);
}