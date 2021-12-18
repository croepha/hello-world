/*

This is a very silly example of doing client/server IO using shared memmory.  This was used in a debate 
over whether or not shared memory between untrusted clients and servers could be done securely.  I maintain
that it can be done securely if you structure your code so that you only read each byte from the the 
untrusted buffer once, and often you can do this fine without even having to do a bunch of extra copies..

I do want to say that is rarely the right tool for the job, AF_UNIX/SOCK_SEQPACKET is a much better tool
for this job, and the spinlocks don't really scale well for idle/intermittent workloads, best save shared 
memory communication for high throughput operations...

*/


// clang -fuse-ld=lld -Werror -Wall -g -O0 -fsanitize=address shmem_chat.cpp -o build/shmem_chat.slow.exec
// clang -fuse-ld=lld -Werror -Wall -g -Ofast shmem_chat.cpp -o build/shmem_chat.exec

#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
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

static void write_buf(char const * fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof buf, fmt, va);
    va_end(va);
    for (char *s = buf; (server_data[write_cursor] = *s); s++) write_cursor++;

}

// Defines buf for scratch space ignore it
// fds is defined as an int* this is an erray of fds_len length
// msg is a pointer that you pass to sendmsg/recvmsg
#define fdlist_msg(buf, fds, msg, fds_len) \
    alignas(alignof(cmsghdr)) char buf[CMSG_SPACE(sizeof(int) * (fds_len))]; \
    msghdr msg; int * fds; __fdlist_msg(buf, sizeof buf, &msg, &fds, (fds_len))
inline static void __fdlist_msg(char * buf, size_t buf_size, msghdr * msg, int ** fds, int number_of_fds) {
        *msg = { .msg_control = buf, .msg_controllen = buf_size };
        struct cmsghdr * cmsg  = CMSG_FIRSTHDR(msg);
        *cmsg = { .cmsg_len = CMSG_LEN(sizeof(int) * number_of_fds), .cmsg_level = SOL_SOCKET, .cmsg_type = SCM_RIGHTS, };
        *fds = (int *) (void*) CMSG_DATA(cmsg);
}

char * map_fd(int fd, size_t size) { return (char *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); }

int main (int argc, char ** argv) { int r;

    if (!argv[1]) {
        fprintf(stderr, "specify `server' or `client'\n");
        exit(-1);
    }

    if (strcmp(argv[1], "server") == 0) {
        int server_data_fd = memfd_create("server", MFD_CLOEXEC);
        ftruncate(server_data_fd, server_data_size);
        server_data = map_fd(server_data_fd, server_data_size);

        // Sockets here are only used to setup the shared memory
        server_socket = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,  0);
        unlink(server_socket_addr.sun_path);
        bind(server_socket, (struct sockaddr*)&server_socket_addr, sizeof server_socket_addr);
        listen(server_socket, 1);

        int last_client = -1;
        for (;;) {
            int client_socket_fd = accept4(server_socket, 0, 0, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (client_socket_fd != -1) {
                if (client_pages_next < client_pages + 1024) {
                    write_buf("\nnew_client %lld connected\n", client_pages_next - client_pages);
                    int client_data_fd = memfd_create("client", MFD_CLOEXEC);
                    ftruncate(client_data_fd, client_data_size);
                    *client_pages_next++ =  map_fd(client_data_fd, client_data_size);

                    fdlist_msg(buf, fdptr, msg, 2);
                    fdptr[0] = server_data_fd;
                    fdptr[1] = client_data_fd;

                    r = sendmsg(client_socket_fd, &msg, 0);

                    close(client_data_fd);                    
                } else {
                    fprintf(stderr, "server full\n");
                }
                close(client_socket_fd);
            } else {
                assert(errno == EAGAIN);
            }

            // All 

            for (int client_i = 0; client_pages + client_i < client_pages_next; client_i++) {
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


        fdlist_msg(buf, fdptr, msg, 2);
        fdptr[0] = -1;
        fdptr[1] = -1;

        r = recvmsg(server_fd, &msg, 0);
        error_check(r);
        if (fdptr[0] == -1) {
            fprintf(stderr, "server full\n");
            exit(-1);
        }
        close(server_fd);

        server_data = map_fd(fdptr[0], server_data_size);
        close(fdptr[0]);
        char * client_data = map_fd(fdptr[1], client_data_size);
        close(fdptr[1]);

        int read_cursor = 0;

        fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

        { // get term settings, and then update them
            static struct termios ts;
            r = tcgetattr(0, &ts);
            ts.c_lflag &= ~ICANON; // disable line buffering
            ts.c_lflag &= ~ECHO;   // disable echo
            r = tcsetattr(0, TCSANOW, &ts);
        }

        for (;;) {
            // This is the main IO loop for a client, the only syscalls we do is to read/write to the stdin/stdout
            // all client/server communication is done with memory operations
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
