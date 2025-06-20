#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>

void send_fd(int sock, int fd) {
    struct msghdr msg = {0};
    char buf[1] = {'F'};
    struct iovec io = { .iov_base = buf, .iov_len = sizeof(buf) };

    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    memset(cmsgbuf, 0, sizeof(cmsgbuf));
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));

    if (sendmsg(sock, &msg, 0) < 0)
        perror("sendmsg");
}

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

    if (recvmsg(sock, &msg, 0) < 0) {
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

int main() {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    pid_t pid = fork();
    if (pid == 0) {
        // Child
        close(sv[1]);
        int fd = recv_fd(sv[0]);

        char buf[256] = {0};
        ssize_t n = read(fd, buf, 255);
        if (n > 0) {
            buf[n] = '\0';
            printf("[Child] Read from secret file:\n%s\n", buf);
        } else {
            perror("[Child] read");
        }

        close(fd);
        close(sv[0]);
        exit(0);
    }

    // Parent
    close(sv[0]);

    const char *home = getenv("HOME");
    char path[512];
    snprintf(path, sizeof(path), "%s/Library/Preferences/com.apple.Safari.plist", home);
    //snprintf(path, sizeof(path), "/Users/Shared/../UserB/Library/Preferences/com.apple.Safari.plist");
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("[Parent] open failed");
        exit(1);
    }

    printf("[Parent] Opened FD %d to %s\n", fd, path);
    send_fd(sv[1], fd);
    close(fd);
    close(sv[1]);
    wait(NULL);
    return 0;
}