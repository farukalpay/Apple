#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/φPostBoot.sock"

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
        fprintf(stderr, "[φPostBootReceiver] Invalid control message\n");
        exit(1);
    }

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

int main() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("[φPostBootReceiver] Connected to socket. Receiving FD...\n");
    int fd = recv_fd(sock);
    printf("[φPostBootReceiver] Received FD %d. Reading...\n", fd);

    char buf[129] = {0};
    ssize_t n = read(fd, buf, 128);
    if (n < 0) {
        perror("read");
        return 1;
    }

    buf[n] = '\0';
    printf("[φPostBootReceiver] Read %zd bytes:\n%s\n", n, buf);

    close(fd);
    close(sock);
    return 0;
}