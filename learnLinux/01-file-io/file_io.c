#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    const char *path = "sample.txt";
    const char text[] = "hello from userspace\n";

    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open for write");
        return 1;
    }

    ssize_t written = write(fd, text, sizeof text - 1);
    if (written < 0) {
        perror("write");
        close(fd);
        return 1;
    }
    if (close(fd) < 0) {
        perror("close after write");
        return 1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open for read");
        return 1;
    }

    char buf[64];
    ssize_t got = read(fd, buf, sizeof buf - 1);
    if (got < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    buf[got] = '\0';
    if (close(fd) < 0) {
        perror("close after read");
        return 1;
    }

    printf("read %zd bytes: %s", got, buf);
    return 0;
}
