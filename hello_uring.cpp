// clang-13 -O0 -glldb -fsanitize=address -fno-exceptions -Wall -Werror -luring hello_uring.cpp -o build/hello_uring.exec
#include <asm-generic/errno-base.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <liburing.h>

#define error_check(v) if ((u_int64_t)v == -1) {perror(#v); assert((u_int64_t)v != -1);}
static int const queue_depth = 4;
static int const block_size = 1<<10;
static int const total_blocks = 8;

char buffers[queue_depth][block_size];
int  q_block_i[queue_depth];

unsigned long nums_in_block = block_size / sizeof(unsigned long);

static_assert(queue_depth <= 64, "");

unsigned long buffers_in_use_bitmask = 0;

int get_open_slot() { return __builtin_ctzl(~ buffers_in_use_bitmask); }

int main () { int r;

    {
        // setup test
        auto f = fopen("build/testfile2", "w");
        for (unsigned long i = 0; i< 1024; i++) {
            fwrite(&i, sizeof i, 1, f);
        }
        fclose(f);    
    }

    auto file_fd = open("build/testfile2", O_RDONLY);

    io_uring ring;
    r = io_uring_queue_init(queue_depth, &ring, 0);
    error_check(r);

    int next_block_to_submit = 0;

    struct iovec vecs[queue_depth];

    while(buffers_in_use_bitmask || next_block_to_submit < total_blocks) {
        for (;;) {
            auto sqe = io_uring_get_sqe(&ring);
            if (!sqe) {
                break;
            }

            auto slot_i = get_open_slot();
            assert(slot_i < queue_depth); 
            buffers_in_use_bitmask |= 1 << slot_i;

            q_block_i[slot_i] = next_block_to_submit++;
            vecs[slot_i] = { .iov_base = buffers[slot_i], .iov_len = block_size};
            io_uring_prep_readv(sqe, file_fd, &vecs[slot_i], 1, slot_i * block_size);
            sqe->user_data = slot_i;
            printf("submit: %d\n", slot_i);
        }

        r = io_uring_submit(&ring);
        error_check(r);
        assert(r == queue_depth);

        {
            struct io_uring_cqe *cqe;
            r = io_uring_wait_cqe(&ring, &cqe);
            error_check(r);
            completion_loop:



            auto block_i = q_block_i[cqe->user_data];
            auto block_start = block_i * nums_in_block;
            auto block_nums = (unsigned long *)buffers[cqe->user_data];


            printf("got_completion: %lld, %d, %d\n", cqe->user_data, cqe->res, cqe->flags);
            printf("slot:%lld block_i:%d block_start:%ld first:%ld\n", cqe->user_data, block_i, block_start, block_nums[0]);


            for (auto num_i = 0; num_i < nums_in_block; num_i++) {
                assert(block_start + num_i == block_nums[num_i]);
            }

            io_uring_cqe_seen(&ring, cqe);

            r = io_uring_peek_cqe(&ring, &cqe);
            if (r != EAGAIN) {
                error_check(r);
                goto completion_loop;
            }
        }
    }
}