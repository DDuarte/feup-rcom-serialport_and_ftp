#include "app.h"
#include "misc.h"
#include "link.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

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
            int16 size;
        } file_size;

        struct file_name
        {
            char* name; // NULL-terminated
        } file_name;
    };
} control_param;

int app_receive_data_packet(link_layer* ll, int* seq_number, char** buffer, int* length)
{
    char* packet_buffer;
    ssize_t total_size = ll_read(ll, &packet_buffer);
    if (total_size < 0)
    {
        perror("ll_read");
        return -1;
    }

    int ctrl = packet_buffer[0];

    if (ctrl != CONTROL_FIELD_DATA)
    {
        fprintf(stderr, "ERROR: Control field received (%d) is not CONTROL_FIELD_DATA", ctrl);
        return -1;
    }

    int seq = packet_buffer[1];
    int16 size;
    size.b[0] = packet_buffer[2];
    size.b[1] = packet_buffer[3];

    *buffer = malloc(size.w);
    memcpy(*buffer, &packet_buffer[4], size.w);
    free(packet_buffer);

    *seq_number = seq;
    *length = size.w;

    return 0;
}

int app_send_data_packet(link_layer* ll, int seq_number, const char* buffer, int length)
{ LOG
    assert(buffer);

    int total_size = 1 + 1 + 2 + length;
    char* packet_buffer = malloc(total_size);

    int16 length16;
    length16.w = length;

    packet_buffer[0] = CONTROL_FIELD_DATA;
    packet_buffer[1] = seq_number;
    packet_buffer[2] = length16.b[0];
    packet_buffer[3] = length16.b[1];
    memcpy(&packet_buffer[4], buffer, length);

    if (!ll_write(ll, packet_buffer, total_size))
    {
        perror("ll_write");
        return -1;
    }

    free(packet_buffer);

    DEBUG_LINE_MSG("successful");
    return 0;
}

int app_receive_control_packet(link_layer* ll, int* ctrl, int n, control_param* params)
{ LOG
    char* buffer = malloc(MAX_DATA_PACKET_SIZE);
    ssize_t size = ll_read(ll, &buffer);
    if (size < 0)
    {
        perror("ll_read");
        return -1;
    }

    *ctrl = buffer[0];

    int read_size = 2; // ctrl + n

    for (int i = 0; i < n; ++i)
    {
        control_param param;
        param.type = buffer[read_size];
        read_size += 1;
        switch (param.type)
        {
            case FIELD_PARAM_TYPE_FILE_SIZE:
            {
                read_size += 1; // ignore size of param
                param.file_size.size.b[0] = buffer[read_size];
                param.file_size.size.b[1] = buffer[read_size + 1];
                read_size += 2;
                break;
            }
            case FIELD_PARAM_TYPE_FILE_NAME:
            {
                int string_length = buffer[read_size];
                read_size += 1;
                param.file_name.name = malloc(string_length + 1);
                memcpy(param.file_name.name, &buffer[read_size], string_length);
                param.file_name.name[string_length] = 0; // NULL terminated
                read_size += string_length;
                break;
            }
        }

        params[i] = param;
    }

    return 0;
}

int app_send_control_packet(link_layer* ll, int ctrl, int n, control_param* params)
{ LOG
    int total_size = 2; // control_field + n

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
    buffer[1] = n;
    int write_size = 2; // control_field + n

    for (int i = 0; i < n; ++i)
    {
        buffer[write_size] = params[i].type;
        write_size += 1;
        switch (params[i].type)
        {
            case FIELD_PARAM_TYPE_FILE_SIZE:
            {
                char* dest = &buffer[write_size];
                dest[0] = 2; // short
                dest[1] = params[i].file_size.size.b[0];
                dest[2] = params[i].file_size.size.b[1];
                write_size += 3;
                break;
            }
            case FIELD_PARAM_TYPE_FILE_NAME:
            {
                char* dest = &buffer[write_size];
                int string_length = strlen(params[i].file_name.name);
                dest[0] = string_length;
                write_size += 1;
                memcpy(&dest[1], params[i].file_name.name, string_length);
                write_size += string_length;
                break;
            }
            default:
            {
                assert(false && "Unknown FIELD_PARAM_TYPE");
                return -1;
            }
        }
    }

    if (!ll_write(ll, buffer, total_size))
    {
        perror("ll_write");
        return -1;
    }

    free(buffer);

    return 0;
}

int get_file_size(FILE* file)
{ LOG
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
{ LOG
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
    paramStartFileSize.file_size.size.w = file_size;

    control_param paramStartFileName;
    paramStartFileName.type = FIELD_PARAM_TYPE_FILE_NAME;
    paramStartFileName.file_name.name = (char*)file_name;

    control_param params[] = { paramStartFileSize, paramStartFileName };

    if (app_send_control_packet(&ll, CONTROL_FIELD_START, 2, params) != 0)
    {
        perror("app_send_control_packet");
        return -1;
    }

    int i = 0;

    char* buffer = malloc(MAX_DATA_PACKET_SIZE);
    size_t read_bytes;
    while ((read_bytes = fread(buffer, sizeof(char), MAX_DATA_PACKET_SIZE, file)) != 0)
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
{ LOG
    // start link layer
    link_layer ll = ll_open(term, RECEIVER);
    if (ll.connection.fd == -1)
    {
        perror("ll_open");
        return -1;
    }

    int ctrlStart;
    control_param startParams[2];
    if (app_receive_control_packet(&ll, &ctrlStart, 2, startParams) != 0)
    {
        perror("app_receive_control_packet (start)");
        return -1;
    }

    if (ctrlStart != CONTROL_FIELD_START)
    {
        fprintf(stderr, "ERROR: Control field received (%d) is not CONTROL_FIELD_START\n", ctrlStart);
        return -1;
    }

    int total_size_read = 0;
    int seq_number;
    while (total_size_read != startParams[0].file_size.size.w)
    {
        char* buffer;
        int length;
        if (app_receive_data_packet(&ll, &seq_number, &buffer, &length) != 0)
        {
            perror("app_receive_data_packet");
            return -1;
        }

        total_size_read += length;
        write(STDOUT_FILENO, buffer, length);
        free(buffer);
    }

    int ctrlEnd;
    control_param endParam;

    if (app_receive_control_packet(&ll, &ctrlEnd, 1, &endParam) != 0)
    {
        perror("app_receive_control_packet (end)");
        return -1;
    }

    if (ctrlEnd != CONTROL_FIELD_END)
    {
        fprintf(stderr, "ERROR: Control field received (%d) is not CONTROL_FIELD_END\n", ctrlEnd);
        return -1;
    }

    if (!ll_close(&ll))
    {
        perror("ll_close");
        return -1;
    }

    return 0;
}
