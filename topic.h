#ifndef _TOPIC_H_
#define _TOPIC_H_

#include "helpers.h"
#include "structures.h"
#include "client.h"

typedef struct topic_struct {
	char name[TOPIC_LEN + 1];

	// different types of subscriptions to this topic
	Elem *clients;
	Elem *sf_clients;
} Topic;

extern Elem *topics_root;

Topic *new_topic(char *name);
Topic *get_topic(char *name);

void subscribe(Client *client, Topic *topic, int sf_flag);
void unsubscribe(Client *client, Topic *topic);

void delete_topic(Elem *elem);
void remove_topics(); // operates on topics_root

#endif