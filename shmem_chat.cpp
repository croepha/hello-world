
// clang-10 -Werror -Wall -g -O0 -fsanitize=address chat.c -o /build/chat.exec

#define _GNU_SOURCE

#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int server_socket = -1;

// We don't track disconnects, so we could easily DOS server, this is easily fixable, but leaving in because this is a toy....
char * client_pages[1024];
int client_read_cursor[1024];
char ** client_pages_next = &client_pages[0];

#define server_data_size 1024
#define client_data_size 1024

struct sockaddr_un const server_socket_addr = {
    .sun_family = AF_UNIX,
    .sun_path   = "/tmp/chat.sock",
};

#define error_check(v) if ((u_int64_t)v == -1) {perror(#v); assert((u_int64_t)v != -1);}

char * server_data;
int write_cursor;

void write_buf(char const * fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof buf, fmt, va);
    va_end(va);
    for (char *s = buf; (server_data[write_cursor] = *s); s++) write_cursor++;

}

int main (int argc, char ** argv) { int r;

    if (strcmp(argv[1], "server") == 0) {
        int server_data_fd = memfd_create("server", MFD_CLOEXEC);
        ftruncate(server_data_fd, server_data_size);
        server_data = mmap(0, server_data_size, PROT_WRITE, MAP_SHARED, server_data_fd, 0);

        server_socket = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,  0);
        unlink(server_socket_addr.sun_path);
        bind(server_socket, (struct sockaddr*)&server_socket_addr, sizeof server_socket_addr);
        listen(server_socket, 1);

        int last_client = -1;
        for (;;) {
            int client_socket_fd = accept4(server_socket, 0, 0,
                                    SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (client_socket_fd != -1) {
                write_buf("\nnew_client %lld connected\n", client_pages_next - client_pages);
                int client_data_fd = memfd_create("client", MFD_CLOEXEC);
                ftruncate(client_data_fd, client_data_size);
                *client_pages_next++ = mmap(0, client_data_size, PROT_READ, MAP_SHARED, client_data_fd, 0);

                _Alignas(_Alignof(struct cmsghdr)) char buf[CMSG_SPACE(sizeof(int) * 2)];
                struct msghdr msg = {
                    .msg_control = buf,
                    .msg_controllen = sizeof(buf),
                };

                struct cmsghdr * cmsg  = CMSG_FIRSTHDR(&msg);
                cmsg->cmsg_level = SOL_SOCKET;
                cmsg->cmsg_type = SCM_RIGHTS;
                cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 2);

                int * fdptr = (int *) (void*) CMSG_DATA(cmsg);
                fdptr[0] = server_data_fd;
                fdptr[1] = client_data_fd;

                r = sendmsg(client_socket_fd, &msg, 0);

                close(client_data_fd);
                close(client_socket_fd);
            } else {
                assert(errno == EAGAIN);
            }

            for (int client_i = 0; client_pages[client_i]; client_i++) {
                while (client_pages[client_i][client_read_cursor[client_i]]) {
                    if (last_client != client_i) {
                        write_buf("\nClient %d: ", client_i);
                        last_client = client_i;
                    }
                    server_data[write_cursor++] = client_pages[client_i][client_read_cursor[client_i]++];
                    write_cursor %= server_data_size;
                    client_read_cursor[client_i] %= client_data_size;
                }
            }
            server_data[write_cursor] = 0;

        }
    } else {
        int server_fd = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC,  0);
        connect(server_fd, (struct sockaddr*)&server_socket_addr, sizeof server_socket_addr);

        _Alignas(_Alignof(struct cmsghdr)) char buf[CMSG_SPACE(sizeof(int) * 2)];
        struct msghdr msg = {
            .msg_control = buf,
            .msg_controllen = sizeof(buf),
        };

        struct cmsghdr * cmsg  = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 2);

        int * fdptr = (int *) (void*) CMSG_DATA(cmsg);
        fdptr[0] = -1;
        fdptr[1] = -1;

        r = recvmsg(server_fd, &msg, 0);
        error_check(r);
        close(server_fd);

        server_data = mmap(0, server_data_size, PROT_READ, MAP_SHARED, fdptr[0], 0);
        close(fdptr[0]);
        char * client_data = mmap(0, client_data_size, PROT_WRITE, MAP_SHARED, fdptr[1], 0);
        close(fdptr[1]);

        int read_cursor = 0;

        fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

        { // get term settings, and then update them
            static struct termios ts;
            r = tcgetattr(0, &ts);
            error_check(r);
            ts.c_lflag &= ~ICANON; // disable line buffering
            ts.c_lflag &= ~ECHO;   // disable echo
            r = tcsetattr(0, TCSANOW, &ts);
            error_check(r);
        }

        for (;;) {
            int written = read(0, client_data + write_cursor, client_data_size - write_cursor);
            if (written > 0) {
                write_cursor = ( write_cursor + written ) % client_data_size;
                client_data[write_cursor] = 0;
            }
            while(server_data[read_cursor]) {
                write(1, server_data + read_cursor++, 1);
                read_cursor %= server_data_size;
            }
        }
    }


}
