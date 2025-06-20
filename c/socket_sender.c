#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/uio.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/Ξ∞.sock"

void send_fd(int conn_fd, int fd_to_send) {
    struct msghdr msg = {0};
    char buf[1] = { 'F' };
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

    if (sendmsg(conn_fd, &msg, 0) == -1) {
        perror("sendmsg");
    } else {
        printf("[φBroadcaster] Sent FD %d\n", fd_to_send);
    }
}

void cleanup_socket() {
    unlink(SOCKET_PATH);
}

int main() {
    signal(SIGINT, (void (*)(int))cleanup_socket);
    cleanup_socket(); // clean if exists

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
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

    printf("[φBroadcaster] Listening on %s...\n", SOCKET_PATH);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        printf("[φBroadcaster] Client connected\n");
        send_fd(client_fd, fd);
        close(client_fd);
    }

    close(fd);
    close(server_fd);
    cleanup_socket();
    return 0;
}