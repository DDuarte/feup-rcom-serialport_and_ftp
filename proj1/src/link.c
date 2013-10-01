#include "link.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static size_t _ll_byte_stuff(char** message, size_t size);
static size_t _ll_byte_destuff(char** message, size_t size);

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
    char* msg;
    size_t bytesWritten;
    int i;

    assert(conn && message);

    msg = compose_message(conn->stat == TRANSMITTER ? ADDR_T_R : ADDR_R_T, message, size, conn->sequence_number);
    conn->sequence_number = conn->sequence_number == 1 ? 0 : 1;

    size += LL_MSG_SIZE_PARTIAL;

    for (i = 0; i < size; ++i)
            printf("0x%X\n", msg[i]);

    printf("\n");

    size = _ll_byte_stuff(&msg, size);

    for (i = 0; i < size; ++i)
        printf("0x%X\n", msg[i]);

    bytesWritten = phy_write(&conn->connection, msg, size);

    // TODO: Timeout -- Temporary Solution: sleep

    sleep(1);

    if (bytesWritten != size)
        perror("Error sending message");

    free(msg);

    return bytesWritten == size;

    return -1;
}

bool ll_send_command(link_layer* conn, ll_cntrl command)
{
    char* cmd;
    size_t bytesWritten;
    size_t messageSize = LL_CMD_SIZE;

    assert(conn);

    cmd = compose_command(conn->stat == TRANSMITTER ? ADDR_T_R : ADDR_R_T, command, conn->sequence_number);
    conn->sequence_number = conn->sequence_number == 1 ? 0 : 1;

    messageSize = _ll_byte_stuff(&cmd, messageSize);

    bytesWritten = phy_write(&conn->connection, cmd, messageSize);

    // TODO: Timeout -- Temporary Solution: sleep

    sleep(1);

    if (bytesWritten != LL_CMD_SIZE)
        perror("Error sending command");

    free(cmd);

    return bytesWritten == LL_CMD_SIZE;
}

#define BUFFER_SIZE 255

ssize_t ll_read(link_layer* conn, char** message)
{
    size_t size = 0;
    ssize_t readRet;
    char c;

    assert(conn);

    do
    {
        readRet = phy_read(&conn->connection, &c, 1);
    } while(readRet == 1 && c != LL_FLAG);

    if (readRet != 1) return -1;

    *message = (char*) malloc(BUFFER_SIZE * sizeof(char));

    (*message)[0] = LL_FLAG;
    size = 1;

    do
    {
        readRet = phy_read(&conn->connection, &c, 1);
        (*message)[size] = c;
        size++;

        if (readRet == 1 && c != LL_FLAG && size % BUFFER_SIZE == 0)
        {
            int mult = size / BUFFER_SIZE + 1;
            *message = (char*) realloc(*message, mult * BUFFER_SIZE);
        }
    } while (readRet == 1 && c != LL_FLAG);

    size = _ll_byte_destuff(message, size);

    return size;

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
    size_t i;
    for (i = 0; i < size; ++i)
        bcc ^= buffer[i];

    return bcc;
}

static size_t _ll_byte_stuff(char** message, size_t size)
{
    size_t i;
    size_t newSize = size;

    for (i = 1; i < size - 1; ++i)
        if ((*message)[i] == LL_FLAG || (*message)[i] == LL_ESC)
            ++newSize;

    *message = (char*)realloc(*message, newSize);

    for (i = 1; i < size - 1; ++i)
    {
        if ((*message)[i] == LL_FLAG || (*message)[i] == LL_ESC)
        {
            memmove(*message+i+1, *message+i, size-i); size++;
            (*message)[i] = LL_ESC;
            (*message)[i+1] ^= LL_CTRL;
        }
    }

    return newSize;
}

static size_t _ll_byte_destuff(char** message, size_t size)
{
    size_t i;

    for (i = 1; i < size - 1; ++i)
    {
        if ((*message)[i] == LL_ESC)
        {
            memmove(*message+i, *message+i+1, size-i-1); size--;
            (*message)[i] ^= LL_CTRL;
        }
    }

    *message = (char*)realloc(*message, size);

    return size;
}

