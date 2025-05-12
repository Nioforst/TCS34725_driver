#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/tcs34725"

// IOCTL definitions (must match kernel driver)
#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_READ_RED   _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 3, int)
#define TCS34725_IOCTL_READ_BLUE  _IOR(TCS34725_IOCTL_MAGIC, 4, int)
#define TCS34725_IOCTL_READ_RGB_NORMALIZED _IOR(TCS34725_IOCTL_MAGIC, 5, struct tcs34725_rgb)

struct tcs34725_rgb {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

int main() {
    int fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    while (1) {
        int r, g, b;
        struct tcs34725_rgb rgb_norm;

        if (ioctl(fd, TCS34725_IOCTL_READ_RED, &r) == -1) {
            perror("Read RED failed");
            break;
        }

        if (ioctl(fd, TCS34725_IOCTL_READ_GREEN, &g) == -1) {
            perror("Read GREEN failed");
            break;
        }

        if (ioctl(fd, TCS34725_IOCTL_READ_BLUE, &b) == -1) {
            perror("Read BLUE failed");
            break;
        }

        if (ioctl(fd, TCS34725_IOCTL_READ_RGB_NORMALIZED, &rgb_norm) == -1) {
            perror("Read normalized RGB failed");
            break;
        }

        printf("Raw RGB: R=%d, G=%d, B=%d | Normalized RGB: R=%u, G=%u, B=%u\n",
            r, g, b, rgb_norm.r, rgb_norm.g, rgb_norm.b);

        sleep(1);
    }

    close(fd);
    return 0;
}