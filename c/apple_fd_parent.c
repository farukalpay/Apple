#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>

void send_fd(int sock, int fd_to_send) {
    struct msghdr msg = {0};
    char buf[1] = { 'F' };
    struct iovec io = { .iov_base = buf, .iov_len = sizeof(buf) };

    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    if (sendmsg(sock, &msg, 0) == -1)
        perror("sendmsg");
}

int main() {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        return 1;
    }

    int fd = open("/Users/farukalpay/Library/Preferences/com.apple.Safari.plist", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child: run under sandbox
        close(sv[1]);

        char sock_fd_str[10];
        snprintf(sock_fd_str, sizeof(sock_fd_str), "%d", sv[0]);

        char *args[] = {
            "sandbox-exec",
            "-f",
            "deny_library.sb",
            "./apple_fd_child",
            sock_fd_str,
            NULL
        };
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    }

    // Parent
    close(sv[0]);
    sleep(1);
    send_fd(sv[1], fd);
    close(fd);
    close(sv[1]);
    wait(NULL);
    return 0;
}