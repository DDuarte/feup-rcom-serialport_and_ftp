// .h

#define CONTROL_FIELD_DATA 0
#define CONTROL_FIELD_START 1
#define CONTROL_FIELD_END 2

#define FIELD_PARAM_TYPE_FILE_SIZE 0
#define FIELD_PARAM_TYPE_FILE_NAME 1
// other field params can be defined




// .c

#define MAX_DATA_PACKET_SIZE 255

#include "link.h"

int app_send_file(const char* file_name)
{
    // open file
    // open connection

    control_param paramStartFileSize;
    paramStartFileSize.type = FIELD_PARAM_TYPE_SIZE_FILE;
    paramStartFileSize.file_size.size = file_size;
    
    control_param paramStartFileName;
    paramStartFileName.type = FIELD_PARAM_TYPE_FILE_NAME;
    paramStartFileName.file_name.name = file_name;
    
    control_param[] params = { paramStartFileSize, param2 };
    
    app_send_control_packet(CONTROL_FIELD_START, 2, params);
    
    // TODO: split file; call multiple sends; increase seq_number

    for (int i = 0; i < file_size / MAX_DATA_PACKET_SIZE; ++i)
    {
        //int buffer_size = file_size < MAX_DATA_PACKET_S
    }
    
    app_send_data_packet(seq_number, file_buffer, buffer_size);
    
    app_send_control_packet(CONTROL_FIELD_END, 0, NULL);
}

int app_send_data_packet(int seq_number, const char* buffer, int length)
{
    assert(buffer);

    int total_size = 1 + 1 + 2 + length;
    char* packet_buffer = malloc(total_size);
    
    packet_buffer[0] = CONTROL_FIELD_DATA;
    packet_buffer[1] = seq_number;
    packet_buffer[2] = length & 0xFF00;
    packet_buffer[3] = length & 0x00FF;
    memcpy(&packet_buffer[4], buffer, length);
    
    llsend(..., packet_buffer, total_size);
}


typedef struct control_param
{
    int type;

    union
    {
        struct file_size
        {
            size_t size;
        };

        struct file_name
        {
            char* name; // NULL-terminated
        };
    };
} control_param;

int app_send_control_packet(int ctrl, int n, control_param* params)
{
    int total_size = 0;
    int i = 0;
    char* buffer;
    int write_size = 0;

    total_size += 1; // control_field
    
    for (i = 0; i < n; ++i)
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
                break;
        }
    }
    
    buffer = malloc(total_size);
    buffer[0] = ctrl;
    write_size = 1;
    
    for (i = 0; i < n; ++i)
    {
        switch (params[i].type)
        {
            case FIELD_PARAM_TYPE_FILE_SIZE:
            {
                char* dest = &buffer[write_size];
                dest[0] = params[i].type;
                dest[1] = 2;
                dest[2] = params[i].file_size & 0xFF00;
                dest[3] = params[i].file_size & 0x00FF;
                write_size += 1 + 1 + 2;
                break;
            }
            case FIELD_PARAM_TYPE_FILE_NAME:
            {
                char* dest = &buffer[write_size];
                int string_length = strlen(params[i].file_name.name);
                dest[0] = params[i].type;
                dest[1] = string_length;
                memcpy(&dest[2], params[i].file_name.name);
                write_size += 1 + 1 + string_length;
                break;
            }
            default:
            {
                assert(false && "Unknown FIELD_PARAM_TYPE");
                break;
            }
        }
    }

    llsend(..., buffer, total_size);
}
