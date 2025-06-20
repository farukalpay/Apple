#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

void send_fd(int sock, int fd_to_send) {
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
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    if (sendmsg(sock, &msg, 0) == -1) {
        perror("sendmsg");
        exit(1);
    }
}

int main() {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        return 1;
    }

    // FD inheritance: remove FD_CLOEXEC so exec'd process keeps sv[0]
    if (fcntl(sv[0], F_SETFD, 0) == -1) {
        perror("fcntl");
        return 1;
    }

    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "No HOME\n");
        return 1;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/Documents/secret_tcc.txt", home);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("[parent] Opened FD %d to %s\n", fd, path);

    pid_t pid = fork();
    if (pid == 0) {
        // Child: exec into xpc_receiver
        char fd_arg[10];
        snprintf(fd_arg, sizeof(fd_arg), "%d", sv[0]);
        char *args[] = { "./xpc_receiver", fd_arg, NULL };
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    }

    // Parent
    close(sv[0]); // Use sv[1] to send
    sleep(1); // wait for child to start and block on recvmsg
    send_fd(sv[1], fd);
    printf("[parent] Sent FD %d to child\n", fd);
    close(fd);
    close(sv[1]);
    return 0;
}