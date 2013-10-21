#include "app.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
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
        printf("Usage:\n");
        printf("\t- send file: %s device file_name\n", argv[0]);
        printf("\t- receive file: %s device\n", argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
