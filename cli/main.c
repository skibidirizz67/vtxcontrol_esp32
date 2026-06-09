#include <stdio.h>
#include <termios.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int open_serial(const char *path, speed_t baud_rate) {
    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return -1;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, baud_rate);
    cfsetospeed(&tty, baud_rate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CLOCAL | CREAD;

    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return -1; }
    return fd;
}

int main(void) {
    int fd = open_serial("/dev/ttyUSB0", B4800);
    if (fd < 0) { perror("open_serial"); return 1; }

    char buf[32];
    while (1) {
        ssize_t r = read(fd, buf, sizeof(buf)-1);
        if (r > 0) {
            buf[r] = '\0';
            printf("RX: %s\n", buf);
        }
        uint8_t tmp[] = {0xAA, 0x55};
        write(fd, tmp, 2);
        getchar();
    }
    return 0;
}