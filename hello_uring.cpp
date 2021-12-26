// clang-13 -O0 -glldb -fsanitize=address -fno-exceptions -Wall -Werror -luring hello_uring.cpp -o build/hello_uring.exec
#include <linux/fs.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <liburing.h>
#include <string.h>
#include <unistd.h>

#define error_check(v) if ((u_int64_t)v == -1) {perror(#v); assert((u_int64_t)v != -1);}
static int const queue_depth = 64;
static int const block_size = 1<<8;
static unsigned long const nums_in_block = block_size / sizeof(unsigned long);
static int const total_numbs = 1<<12;
static int const total_blocks = total_numbs / nums_in_block;

char buffers[queue_depth][block_size];
int  q_block_i[queue_depth];


static_assert(queue_depth <= 64, "");

unsigned long buffers_in_use_bitmask = 0;

int get_open_slot() { return __builtin_ctzl(~ buffers_in_use_bitmask); }

int main (int, char** args) { int r;

    bool args_do_fill = 0;
    bool args_do_dump = 0;
    bool args_do_method0 = 0;
    bool args_do_method1 = 0;
    bool args_err = 0;
    bool args_short = 0;
    bool args_do_plain = 0;

    while (*++args) {
        if (strcmp(*args, "fill") == 0) {
            args_do_fill = 1;
        } else if (strcmp(*args, "dump") == 0) {
            args_do_dump = 1;
        } else if (strcmp(*args, "method0") == 0) {
            args_do_method0 = 1;
        } else if (strcmp(*args, "method1") == 0) {
            args_do_method1 = 1;
        } else if (strcmp(*args, "short") == 0) {
            args_short = 1;
        } else if (strcmp(*args, "plain") == 0) {
            args_do_plain = 1;
        } else {
            fprintf(stderr, "bad option: %s\n", *args);
            args_err = 1;
        }
    }
    if (args_err) {
        exit(-1);
    }

    if (args_do_fill) {
        // setup test
        auto f = fopen("build/testfile2", "w");
        for (unsigned long i = 0; i< total_numbs; i++) {
            fwrite(&i, sizeof i, 1, f);
        }
        fclose(f);
    }


    if (args_do_dump) {
        // setup test
        auto f = fopen("build/testfile2", "r");
        int i = 0;
        for (;;) {
            unsigned long value;
            auto offset = ftell(f);
            r = fread(&value, sizeof value, 1, f);
            error_check(r);
            if (r == 0) {
                break;
            }
            printf("%ld %d %ld\n", offset, i++, value);
        }
        fclose(f);
    }
    // exit(-1);

    auto file_fd = open("build/testfile2", O_RDONLY);

    io_uring ring;
    r = io_uring_queue_init(queue_depth, &ring, 0);
    error_check(r);



    auto start_tsc = __builtin_ia32_rdtsc();

    if (args_do_method0) {
        for (int iteration_i = 0; iteration_i < 64; iteration_i++) {

            int next_block_to_submit = 0;
            struct iovec vecs[queue_depth];
            while(buffers_in_use_bitmask || next_block_to_submit < total_blocks) {
                for (;;) {
                    auto sqe = io_uring_get_sqe(&ring);
                    if (!sqe) {
                        break;
                    }

                    auto block_i = next_block_to_submit++;
                    auto slot_i = get_open_slot();
                    assert(slot_i < queue_depth);
                    buffers_in_use_bitmask |= 1 << slot_i;

                    q_block_i[slot_i] = block_i;
                    vecs[slot_i] = { .iov_base = buffers[slot_i], .iov_len = block_size};
                    io_uring_prep_readv(sqe, file_fd, &vecs[slot_i], 1, block_i * block_size);
                    sqe->user_data = slot_i;
                    //printf("submit slot:%d block_i:%d \n", slot_i, block_i);
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

                    //printf("got_completion: %lld, %d, %d\n", cqe->user_data, cqe->res, cqe->flags);
                    //printf("slot:%lld block_i:%d block_start:%ld first:%ld\n", cqe->user_data, block_i, block_start, block_nums[0]);


                    for (auto num_i = 0; num_i < nums_in_block; num_i++) {
                        assert(block_start + num_i == block_nums[num_i]);
                    }

                    buffers_in_use_bitmask &= ~(1 << cqe->user_data);
                    io_uring_cqe_seen(&ring, cqe);

                    r = io_uring_peek_cqe(&ring, &cqe);
                    if (r == EAGAIN) {
                        r = 0;
                        cqe = 0;
                    }
                        error_check(r);
                    if (cqe) {
                        goto completion_loop;
                    }
                }
            }
        }
    }
    if (args_do_method1) {
        for (int iteration_i = 0; iteration_i < 64; iteration_i++) {

            int next_block_to_submit = 0;
            if (args_short) {
                next_block_to_submit = total_blocks - 1024;
            }
            struct iovec vecs[queue_depth];

            for (int slot_i = 0; slot_i < queue_depth; slot_i++) {
                auto block_i = next_block_to_submit++;
                auto sqe = io_uring_get_sqe(&ring);
                assert(sqe);
                assert(slot_i < queue_depth);
                buffers_in_use_bitmask |= 1 << slot_i;

                q_block_i[slot_i] = block_i;
                vecs[slot_i] = { .iov_base = buffers[slot_i], .iov_len = block_size};
                io_uring_prep_readv(sqe, file_fd, &vecs[slot_i], 1, block_i * block_size);
                sqe->user_data = slot_i;
                // printf("submit slot:%d block_i:%d \n", slot_i, block_i);

            }
            r = io_uring_submit(&ring);
            error_check(r);
            int active_slots = queue_depth;
            while(active_slots || next_block_to_submit < total_blocks) {

                struct io_uring_cqe * cqe;
                r = io_uring_peek_cqe(&ring, &cqe);
                if (r == EAGAIN) {
                    r = 0;
                    cqe = 0;
                }
                error_check(r);
                if (!cqe) {
                    // r = io_uring_submit_and_wait(&ring, 1);
                    // error_check(r);
                    r = io_uring_submit(&ring);
                    error_check(r);
                    r = io_uring_wait_cqe(&ring, &cqe);
                    error_check(r);
                }


                auto slot_i = cqe->user_data;

                {
                auto complete_block_i = q_block_i[slot_i];
                auto complete_block_start = complete_block_i * nums_in_block;
                auto complete_block_nums = (unsigned long *)buffers[slot_i];
                auto complete_read_result = cqe->res;

                // printf("got_completion: %lld, %d, %d\n", cqe->user_data,
                // cqe->res, cqe->flags); printf("slot:%lld block_i:%d
                // block_start:%ld first:%ld\n", cqe->user_data, block_i,
                // block_start, block_nums[0]);

                io_uring_cqe_seen(&ring, cqe);

                assert(complete_read_result > 0);

                for (auto num_i = 0; num_i < nums_in_block; num_i++) {
                    assert(complete_block_start + num_i ==
                        complete_block_nums[num_i]);
                    }
                }

                if (next_block_to_submit < total_blocks) {

                    auto sqe = io_uring_get_sqe(&ring);
                    assert(sqe);

                    auto submit_block_i = next_block_to_submit++;
                    buffers_in_use_bitmask |= 1 << slot_i;

                    q_block_i[slot_i] = submit_block_i;
                    vecs[slot_i] = { .iov_base = buffers[slot_i], .iov_len = block_size};
                    io_uring_prep_readv(sqe, file_fd, &vecs[slot_i], 1,
                                        submit_block_i * block_size);
                    sqe->user_data = slot_i;
                    // printf("submit slot:%llu block_i:%d \n", slot_i, block_i);
                } else {
                    active_slots--;
                }

                // r = io_uring_submit(&ring);
                // error_check(r);

            }
        }

    }

    if (args_do_plain) {
        for (int iteration_i = 0; iteration_i < 64; iteration_i++) {
          r = lseek(file_fd, 0, SEEK_SET);
          error_check(r);

          for (int block_i = 0; block_i < total_blocks; block_i++) {
            char buf[block_size];
            r = read(file_fd, buf, sizeof buf);
            error_check(r);
            assert(r == block_size);

            auto block_start = block_i * nums_in_block;
            auto block_nums = (unsigned long *)buf;

            for (auto num_i = 0; num_i < nums_in_block; num_i++) {
              assert(block_start + num_i == block_nums[num_i]);
            }
          }
        }
    }

    auto end_tsc = __builtin_ia32_rdtsc();
    printf("delta tsc: %lld\n", end_tsc - start_tsc);

}