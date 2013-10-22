#include "app.h"
#include "misc.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{ LOG
    if (argc == 2) // receiver
    {
        if (app_receive_file(argv[1]) != 0)
        {
            perror("app_receive_file");
            return EXIT_FAILURE;
        }
    }
    else if (argc == 3) // sender, i.e "./rcom /dev/ttyS4 pinguim.gif"
    {
        if (app_send_file(argv[1], argv[2]) != 0)
        {
            perror("app_send_file");
            return EXIT_FAILURE;
        }
    }
    else
    {
        ERROR("Usage:");
        ERRORF("\t- send file: %s device in_file_name", argv[0]);
        ERRORF("\t- receive file: %s device > out_file_name", argv[0]);
        ERROR("Note: errors and other messages are printed to stderr");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
