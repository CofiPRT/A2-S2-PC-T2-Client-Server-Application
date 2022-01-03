#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define MAX_CLIENTS			SOMAXCONN // linux value = 128
#define SOCKADDR_SIZE		sizeof(struct sockaddr)

#define BUFLEN				256
#define COMMAND_LEN			100
#define ID_LEN				10
#define TOPIC_LEN			50
#define PAYLOAD_LEN			1500
#define UDP_LEN				(TOPIC_LEN + 1 + PAYLOAD_LEN)
#define TYPE_LEN			10

#define MESS_LEN			1590
/* EXPLANATION
 * Longest IP: ###.###.###.###	3+1+3+1+3+1+3	15 chars
 * Character ':'				1				1 char
 * Longest Port: #####			5				5 chars
 * String " - "					1+1+1			3 chars
 * Topic: Fixed 50 chars		TOPIC_LEN		50 chars
 * String " - "					1+1+1			3 chars
 * Longest Type: "SHORT_REAL"	10				10 chars
 * String " - "					1+1+1			3 chars
 * Payload 						PAYLOAD_LEN		1500 chars
 *********************************************************
 * TOTAL						1590			40 + TOPIC_LEN + PAYLOAD_LEN
 */


#define TRUE				1
#define FALSE				0

#define USE_NEAGLE			FALSE
#define LOG					FALSE
#define CHANGEABLE_SUB		TRUE
#define VALIDATE_COMMANDS	TRUE


// if condition -> display message AND exit program
#define DIE(assertion, fmt, ...)						\
	do {												\
		if (assertion) {								\
			fprintf(stderr, "(%s, %d): " fmt,			\
					__FILE__, __LINE__, ## __VA_ARGS__);\
			exit(EXIT_FAILURE);							\
		}												\
	} while(0)

// if condition -> display message (DON'T EXIT PROGRAM)
#define DIED(assertion, fmt, ...)						\
	DIED_RESULT = FALSE;								\
	do {												\
		if (assertion) {								\
			DIED_RESULT = TRUE;							\
			fprintf(stderr, "(%s, %d): " fmt,			\
					__FILE__, __LINE__, ## __VA_ARGS__);\
		}												\
	} while(0)

extern int DIED_RESULT;

// if logging is enabled, log the message to stdout
#define GOOD(fmt, ...)									\
	do {												\
		if (LOG) {										\
			printf("(%s, %d): " fmt,					\
					__FILE__, __LINE__, ## __VA_ARGS__);\
		}												\
	} while(0)

int max(int a, int b);

#endif
