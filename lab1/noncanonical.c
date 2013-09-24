/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

int main(int argc, char** argv)
{
    int fd, c, res, j;
    struct termios oldtio, newtio;
    char buf[255];

    if (argc < 2) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if (tcgetattr(fd,&oldtio) != 0) { /* save current port settings */
    printf("tcgetattr oldtio failed");
    exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd,TCSANOW,&newtio) != 0) {
    printf("tcgetattr newtio failed");
    exit(-1);
    }

    printf("New termios structure set\n");

    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,buf,255);   /* returns after 1 chars have been input */
      buf[res]=0;               /* so we can printf... */
      printf("%s", buf);
    if (errno != EAGAIN) printf("\n");
      //for (j=0;j<strlen(buf);++j) printf("0x%X\n", buf[j]);
      if (buf[0]=='z') STOP=TRUE;
      memset(buf, 0, 255);
    }
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
