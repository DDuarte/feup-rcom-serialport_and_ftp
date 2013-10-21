#include "link.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "misc.h"

#define CONTROL_FIELD_DATA 0
#define CONTROL_FIELD_START 1
#define CONTROL_FIELD_END 2

#define FIELD_PARAM_TYPE_FILE_SIZE 0
#define FIELD_PARAM_TYPE_FILE_NAME 1
// other field params can be defined

#define MAX_DATA_PACKET_SIZE 255

typedef struct control_param
{
    int type;

    union
    {
        struct file_size
        {
            size_t size;
        } file_size;

        struct file_name
        {
            char* name; // NULL-terminated
        } file_name;
    };
} control_param;

int app_send_data_packet(link_layer* ll, int seq_number, const char* buffer, int length)
{
    assert(buffer);

    int total_size = 1 + 1 + 2 + length;
    char* packet_buffer = malloc(total_size);

    packet_buffer[0] = CONTROL_FIELD_DATA;
    packet_buffer[1] = seq_number;
    packet_buffer[2] = (char)(length & 0xFF00);
    packet_buffer[3] = (char)(length & 0x00FF);
    memcpy(&packet_buffer[4], buffer, length);

    if (ll_write(ll, packet_buffer, total_size) == -1)
    {
        perror("ll_write");
        return -1;
    }

    DEBUG_LINE_MSG("successful");
    return 0;
}

int app_send_control_packet(link_layer* ll, int ctrl, int n, control_param* params)
{
    int total_size = 1; // control_field

    for (int i = 0; i < n; ++i)
    {
        switch (params[i].type)
        {
            case FIELD_PARAM_TYPE_FILE_SIZE:
                total_size += 1 + 1 + 2;
                break;
            case FIELD_PARAM_TYPE_FILE_NAME:
                total_size += 1 + 1 + strlen(params[i].file_name.name);
                break;
            default:
                assert(false && "Unknown FIELD_PARAM_TYPE");
                return -1;
        }
    }

    char* buffer = malloc(total_size);
    buffer[0] = ctrl;
    int write_size = 1; // control_field

    for (int i = 0; i < n; ++i)
    {
        switch (params[i].type)
        {
            case FIELD_PARAM_TYPE_FILE_SIZE:
            {
                char* dest = &buffer[write_size];
                dest[0] = params[i].type;
                dest[1] = 2;
                dest[2] = params[i].file_size.size & 0xFF00;
                dest[3] = params[i].file_size.size & 0x00FF;
                write_size += 1 + 1 + 2;
                break;
            }
            case FIELD_PARAM_TYPE_FILE_NAME:
            {
                char* dest = &buffer[write_size];
                int string_length = strlen(params[i].file_name.name);
                dest[0] = params[i].type;
                dest[1] = string_length;
                memcpy(&dest[2], params[i].file_name.name, string_length);
                write_size += 1 + 1 + string_length;
                break;
            }
            default:
            {
                assert(false && "Unknown FIELD_PARAM_TYPE");
                return -1;
            }
        }
    }

    if (ll_write(ll, buffer, total_size) == -1)
    {
        perror("ll_write");
        return -1;
    }

    return 0;
}

int get_file_size(FILE* file)
{
    int file_size;
    if (fseek(file, 0L, SEEK_END) == -1)
    {
        perror("fseek");
        return -1;
    }

    file_size = ftell(file);
    if (fseek(file, 0L, SEEK_SET) == -1)
    {
        perror("fseek");
        return -1;
    }

    return file_size;
}

int app_send_file(const char* term, const char* file_name)
{
    // open file
    FILE* file = fopen(file_name, "rb");
    if (!file)
    {
        perror("fopen");
        return -1;
    }

    // start link layer
    link_layer ll = ll_open(term, TRANSMITTER);
    if (ll.connection.fd == -1)
    {
        perror("ll_open");
        return -1;
    }

    int file_size = get_file_size(file);
    if (file_size == -1)
    {
        perror("get_file_size");
        return -1;
    }

    // build control params
    control_param paramStartFileSize;
    paramStartFileSize.type = FIELD_PARAM_TYPE_FILE_SIZE;
    paramStartFileSize.file_size.size = file_size;

    control_param paramStartFileName;
    paramStartFileName.type = FIELD_PARAM_TYPE_FILE_NAME;
    paramStartFileName.file_name.name = file_name;

    control_param params[] = { paramStartFileSize, paramStartFileName };

    if (app_send_control_packet(&ll, CONTROL_FIELD_START, 2, params) != 0)
    {
        perror("app_send_control_packet");
        return -1;
    }

    int i = 0;

    char* buffer = malloc(MAX_DATA_PACKET_SIZE);
    size_t read_bytes;
    while ((read_bytes = fread(buffer, sizeof(char), MAX_DATA_PACKET_SIZE, file) != 0))
    {
        if (app_send_data_packet(&ll, (i++) % 255, buffer, read_bytes) == -1)
        {
            perror("app_send_data_packet");
            return -1;
        }

        buffer = memset(buffer, 0, MAX_DATA_PACKET_SIZE);
    }

    free(buffer);

    if (fclose(file) != 0)
    {
        perror("fclose");
        return -1;
    }

    if (app_send_control_packet(&ll, CONTROL_FIELD_END, 0, NULL) != 0)
    {
        perror("app_send_control_packet");
        return -1;
    }

    if (!ll_close(&ll))
    {
        perror("ll_close");
        return -1;
    }

    return 0;
}

int app_receive_file(const char* term)
{
    errno = ENOSYS;
    return -1;
}
