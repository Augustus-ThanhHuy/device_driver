#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define DEVICE_FILE "/dev/gpio_dev"

int main()
{
    int fd;
    struct pollfd pfd;

    fd = open(DEVICE_FILE, O_RDONLY); //Chỉ đọc data nên để  O_RDONLY
    if (fd < 0) {
        perror("Failed to open device file");
        return 1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;

    //Cơ chế polling, đợi khi nào hàm read thoát(nghĩa là có ngắt) và in ra.
    while (1) {
        int ret = poll(&pfd, 1, -1);
        if (ret > 0) {
            if (pfd.revents & POLLIN) {
                char buf[10];
                read(fd, buf, sizeof(buf));
                printf("Interrupt received\n");
            }
        } else {
            perror("Poll error");
            break;
        }
    }

    close(fd);
    return 0;
}