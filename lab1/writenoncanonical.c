/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    char * input;
    int fd,c, res;
    size_t input_size = 0;
    struct termios oldtio,newtio;
    char buf[255];
    int i, j, sum = 0, speed = 0;

    if ( (argc < 2) /*||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )*/) {
      printf("Usage:\t%s SerialPort\n\tex: nserial /dev/ttyS1\n", argv[1]);
      exit(1);
    }

    if ((fd = open(argv[1], O_RDWR | O_NOCTTY )) <0)
    {
        perror(argv[1]);
        exit(-1);
    }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    /* CONFIG START */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;d

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    /* CONFIG END */

    do {
        printf("Please insert the message: ");
    } while ((input_size = getline(&input, &input_size, stdin)) <= 0);

    input[input_size-1] = 0;

    printf("input_size: %lu\n", input_size);
    printf("strlen(input): %lu\n", strlen(input));

    printf("Writing '%s' to serial.\n", input);

    for (j = 0; j < strlen(input); ++j) {
        printf("0x%X\n", input[j]);
    }

    res = write(fd,input,strlen(input)+1);
    printf("%d bytes written\n", res);

    if (input) free(input);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);

    return 0;
}
