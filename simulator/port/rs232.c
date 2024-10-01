#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include "bsp/rs232.h"
#include "services/timestamp.h"


static int  open_tty(char *portname);
static void setup_port(int fd);
int         set_interface_attribs(int fd, int speed);
void        set_mincount(int fd, int mcount);


static int port_fd = 0;


void bsp_rs232_init(void) {
    if ((port_fd = open_tty("/dev/ttyUSB0")) > 0) {
        printf("Porta trovata\n");
        setup_port(port_fd);
    } else {
        if ((port_fd = open_tty("/dev/ttyUSB1")) > 0) {
            printf("Porta trovata\n");
            setup_port(port_fd);
        } else {
            printf("Porta non trovata\n");
        }
    }
}


void bsp_rs232_flush(void) {
    tcflush(port_fd, TCIFLUSH);
}


int bsp_rs232_read(uint8_t *buffer, size_t required_len) {
    int           len;
    unsigned long start = timestamp_get();
    size_t        total = 0;

    do {
        len = read(port_fd, &buffer[total], required_len);

        if (len > 0) {
            total += len;
        } else {
        }
    } while (!timestamp_is_expired(start, 50));

    return total;
}


int bsp_rs232_write(uint8_t *buffer, size_t len) {
    int wlen = 0, res;

    do {
        if ((res = write(port_fd, buffer, len - wlen)) < 0) {
            printf("Errore nella scrittura: %s\n", strerror(errno));
            return len - wlen;
        }

        wlen -= res;
    } while (wlen > 0);

    return len;
}


static int open_tty(char *portname) {
    return open(portname, O_RDWR | O_NOCTTY | O_SYNC);
}


static void setup_port(int fd) {
    set_interface_attribs(fd, B230400);
    set_mincount(fd, 0);
    tcflush(fd, TCIFLUSH);
}


int set_interface_attribs(int fd, int speed) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      /* 8-bit characters */
    tty.c_cflag &= ~PARENB;  /* no parity bit */
    tty.c_cflag &= ~CSTOPB;  /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


void serial_set_timeout(int fd, int mcount, int decsec) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN]  = mcount;
    tty.c_cc[VTIME] = decsec;

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}


void set_mincount(int fd, int mcount) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN]  = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}
