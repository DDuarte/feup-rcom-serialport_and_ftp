#ifndef LINK_H_
#define LINK_H_

#include <stdbool.h>

#include "physical.h"

#define MAX_FRAME_SIZE 1337

#define FLAG 0x7E
#define ESC  0x7D
#define CTRL 0x20

typedef enum
{
	ADDR_RECEIVER_TO_TRANSMITTER = 0x03,
	ADDR_TRANSMITTER_TO_RECEIVER = 0x01
} ll_address;

typedef enum
{
	FRAME_INFO,
	FRAME_SUPER,
	FRAME_NOT_NUMERED
} ll_frame_type;


typedef enum
{
	TRANSMITTER,
	RECEIVER
} ll_status;

typedef struct
{
	phy_connection connection;

	unsigned int sequence_number;
	unsigned int timeout;
	unsigned int number_transmissions;

	char frame[MAX_FRAME_SIZE];
} link_layer;

link_layer ll_open(const char* term, ll_status stat);
ssize_t ll_write(link_layer* conn, const char* message, size_t size);
ssize_t ll_read(link_layer* conn, char** message);
bool ll_close(link_layer* conn);

#endif /* LINK_H_ */
