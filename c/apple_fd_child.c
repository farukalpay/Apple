#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>

int recv_fd(int sock) {
    struct msghdr msg = {0};
    char dummy;
    struct iovec iov = { &dummy, 1 };
    char buf[CMSG_SPACE(sizeof(int))];
    memset(buf, 0, sizeof(buf));

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(sock, &msg, 0) == -1) {
        perror("recvmsg");
        exit(1);
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS) {
        fprintf(stderr, "Invalid control message\n");
        exit(1);
    }

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <socket-fd>\n", argv[0]);
        return 1;
    }

    int sock = atoi(argv[1]);
    int fd = recv_fd(sock);

    char buf[129] = {0};
    ssize_t n = read(fd, buf, 128);
    if (n < 0) {
        perror("read");
        return 1;
    }

    buf[n] = '\0';
    printf("[Child] Read from secret file:\n%s\n", buf);

    close(fd);
    close(sock);
    return 0;
}