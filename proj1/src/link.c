#include "link.h"

#include <assert.h>
#include <stdio.h>

link_layer ll_open(const char* term, ll_status stat)
{
	link_layer ll;

	ll.connection = phy_open(term);

	if(ll.connection.fd < 0)
		return ll;

	ll.stat = stat;
	ll.number_transmissions = 0;
	ll.sequence_number = 0;
	ll.timeout = 0;

	return ll;
}

ssize_t ll_write(link_layer* conn, const char* message, size_t size)
{
	assert(conn && message);

	return -1;
}

ssize_t ll_read(link_layer* conn, char** message)
{
	assert(conn);

	return -1;

}

bool ll_close(link_layer* conn)
{
	assert(conn);

	return phy_close(&conn->connection);
}

char * compose_command(ll_address address, ll_cntrl cntrl, int n)
{
	char * command = malloc(LL_CMD_SIZE);
	command[0] = LL_FLAG;
	command[1] = address;

	switch (cntrl)
	{
	case CNTRL_RR:
	case CNTRL_REJ:
		command[2] = cntrl | (n << 6);
		break;
	default:
		command[2] = cntrl;
		break;
	}

	command[3] = ll_calculate_bcc(&command[1], 2);
	command[4] = LL_FLAG;

	return command;
}

char* compose_message(ll_address address, const char* msg, size_t size, int ns)
{
	char* message = malloc(LL_MSG_SIZE_PARTIAL + size);

	message[0] = LL_FLAG;
	message[1] = address;
	message[2] = ns << 2;
	message[3] = ll_calculate_bcc(&message[1], 2);

	memcpy(&message[3], msg, size);

	message[3 + size] = ll_calculate_bcc(msg, size);
	message[4 + size] = LL_FLAG;

	return message;
}

char ll_calculate_bcc(const char* buffer, size_t size)
{
	char bcc = 0;
	for (size_t i = 0; i < size; ++i)
		bcc ^= buffer[i];

	return bcc;
}
