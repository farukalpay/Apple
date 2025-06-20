/*  fd_transfer_demo.c
 *
 *  Demonstrates “phantom” capability flow on macOS:
 *  1. Create a Unix-domain socket pair between parent & child.
 *  2. Parent opens /private/etc/passwd (or any readable file).
 *  3. Parent sends the open file descriptor to the child via SCM_RIGHTS.
 *  4. Child reads from the inherited descriptor—even if sandboxed or
 *     lacking entitlements to open the file directly.
 *
 *  Compile:  cc fd_transfer_demo.c -o fd_transfer_demo
 *  Run:      ./fd_transfer_demo
 */

 #define _GNU_SOURCE
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <sys/uio.h>
 #include <sys/un.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <sys/wait.h>
 
 /* ──────────────────────────────────────────  Helpers  ── */
 
 static void die(const char *msg)
 {
     perror(msg);
     exit(EXIT_FAILURE);
 }
 
 /* Send one file descriptor over a Unix domain socket */
 static void send_fd(int sock, int fd_to_send)
 {
     struct msghdr msg  = {0};
     struct iovec  iov  = { (void *)"F", 1 };
     char          buf[CMSG_SPACE(sizeof(int))];
     memset(buf, 0, sizeof(buf));
 
     msg.msg_iov        = &iov;
     msg.msg_iovlen     = 1;
     msg.msg_control    = buf;
     msg.msg_controllen = sizeof(buf);
 
     struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
     cmsg->cmsg_level = SOL_SOCKET;
     cmsg->cmsg_type  = SCM_RIGHTS;
     cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
     memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));
 
     if (sendmsg(sock, &msg, 0) == -1)
         die("sendmsg");
 }
 
 /* Receive one file descriptor from a Unix domain socket */
 static int recv_fd(int sock)
 {
     struct msghdr msg = {0};
     char dummy;
     struct iovec  iov = { &dummy, 1 };
     char          buf[CMSG_SPACE(sizeof(int))];
     memset(buf, 0, sizeof(buf));
 
     msg.msg_iov        = &iov;
     msg.msg_iovlen     = 1;
     msg.msg_control    = buf;
     msg.msg_controllen = sizeof(buf);
 
     printf("[child ] about to call recvmsg...\n");
     if (recvmsg(sock, &msg, 0) == -1) {
         perror("recvmsg failed");
         printf("msg_control: %p\n", msg.msg_control);
         printf("msg_controllen: %u\n", msg.msg_controllen);
         exit(EXIT_FAILURE);
     }
 
     printf("[child ] recvmsg succeeded.\n");
 
     struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
     if (cmsg == NULL) {
         fprintf(stderr, "CMSG_FIRSTHDR returned NULL\n");
         exit(EXIT_FAILURE);
     }
 
     if (cmsg->cmsg_type != SCM_RIGHTS) {
         fprintf(stderr, "unexpected cmsg type: %d\n", cmsg->cmsg_type);
         exit(EXIT_FAILURE);
     }
 
     int received_fd;
     memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(int));
     return received_fd;
 }
 /* ──────────────────────────────────────────  Main  ── */
 
 int main(void)
 {
     int sv[2];                             /* sv[0]↔sv[1] full-duplex */
     if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)
         die("socketpair");
 
     pid_t pid = fork();
     if (pid == -1)
         die("fork");
 
     /* ── Parent: open file + send FD ─────────────────── */
     if (pid > 0) {
         close(sv[0]);                      /* parent uses sv[1] */
         int fd = open("/Users/farukalpay/Documents/secret_tcc.txt", O_RDONLY);
         if (fd == -1)
             die("open /private/etc/passwd");
 
         send_fd(sv[1], fd);
         printf("[parent] sent FD %d to child\n", fd);
 
         close(fd);
         close(sv[1]);
         wait(NULL);                        /* reap child */
     }
 
     /* ── Child: receive FD + read ────────────────────── */
     else {
         close(sv[1]);                      /* child uses sv[0] */
         int fd = recv_fd(sv[0]);
         printf("[child ] received FD %d  — reading 128 bytes:\n", fd);
 
         char buffer[129];
         ssize_t n = read(fd, buffer, 128);
         if (n < 0)
             die("read");
         buffer[n] = '\0';
         printf("----------------------------------------\n%s\n----------------------------------------\n", buffer);
 
         close(fd);
         close(sv[0]);
     }
     return 0;
 }