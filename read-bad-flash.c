/*
 * read-bad-flash
 *
 * This little tool can help recover a file from a faulty device.
 * It will try to extract data by reading the input file in chunks of
 * given size even if there are some types of hardware read errors.
 *
 * When the tool encounters a broken chunk it will ask the user whether
 * they want to retry reading or create an empty (zero-filled) chunk
 * instead.
 *
 * https://github.com/0xebef/read-bad-flash
 *
 * License: GPLv3 or later
 *
 * Copyright (c) 2015, 0xebef
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define ARGS_MIN                (3)
#define USER_INPUT_MAX          (16U)
#define CHUNK_SIZE_DEFAULT      (1000000U)

enum {
    ARG_SELF = 0,
    ARG_FILE_INPUT,
    ARG_FILE_OUTPUT,
    ARG_CHUNK_SIZE,
    ARG_START_OFFSET,
    ARG_END_OFFSET
} args_e;

int main(int argc, char *argv[]) {
    FILE *fi = NULL;            /* input file */
    FILE *fo = NULL;            /* output file */
    size_t start;               /* input file start offset */
    size_t end;                 /* input file end offset */
    size_t chunk;               /* chunk size */
    size_t i;
    unsigned char *buf;
    bool is_always_z = false;

    /* input parameters count validation and help message */
    if (argc < ARGS_MIN) {
        fprintf(stdout, "usage: %s <in-file> <out-file> "
               "[chunk-size] [start-offset] [end-offset]\n",
               argv[ARG_SELF]);
        return EXIT_SUCCESS;
    }

    /*
     * input reading and validation
     */

    if (strlen(argv[ARG_FILE_OUTPUT]) > PATH_MAX) {
        fprintf(stderr, "<out-file> is too long\n");
        return EXIT_FAILURE;
    }

    if (argc > ARG_CHUNK_SIZE) {
        chunk = strtoull(argv[ARG_CHUNK_SIZE], NULL, 10);
        if (chunk == 0U) {
            fprintf(stderr, "[chunk-size] can not be zero\n");
            return EXIT_FAILURE;
        }
    } else {
        chunk = CHUNK_SIZE_DEFAULT;
    }

    if (argc > ARG_START_OFFSET) {
        start = strtoull(argv[ARG_START_OFFSET], NULL, 10);
    } else {
        start = 0U;
    }

    if (argc > ARG_END_OFFSET) {
        end = strtoull(argv[ARG_END_OFFSET], NULL, 10);
    } else {
        end = 0U;
    }

    if (end != 0U && start >= end) {
        fprintf(stdout, "nothing to do\n");
        return EXIT_SUCCESS;
    }

    /* allocating working buffer for the chunks of data to read */
    buf = malloc(chunk);
    if (!buf) {
        fprintf(stderr, "can not allocate memory, "
               "try to use a smaller value for [chunk-size]\n");
        return EXIT_FAILURE;
    }

    /* the main loop */
    i = 0U;
    for (;;) {
        size_t rs;
        char user_input[USER_INPUT_MAX];

        /*
         * if the input file is not open then try to open it and seek
         * into the desired position
         */
        if (!fi) {
            fi = fopen(argv[ARG_FILE_INPUT], "rb");
            if (!fi) {
                fprintf(stderr, "can not open the input file\n");
                if (fo) {
                    fclose(fo);
                }
                free(buf);
                return EXIT_FAILURE;
            }
            fseek(fi, i * chunk + start, SEEK_SET);
        }

        /* if the output file is not open then try to open it */
        if (!fo) {
            fo = fopen(argv[ARG_FILE_OUTPUT], "wb");
            if (!fo) {
                fprintf(stderr, "can not create an output file\n");
                fclose(fi);
                free(buf);
                return EXIT_FAILURE;
            }
        }

        /* try to read a chunk from the input file */
        fprintf(stdout, "trying to read %zu bytes at %zu... ",
                chunk, i * chunk + start);
        rs = fread(buf, 1U, chunk, fi);
        if (rs == 0U) {
            fprintf(stdout, "no bytes were read\n");
        } else if (rs == 1U) {
            fprintf(stdout, "1 byte was read\n");
        } else {
            fprintf(stdout, "%zu bytes were read\n", rs);
        }

        if (rs == chunk) {
            /* normal scenario */

            if (fwrite(buf, chunk, 1U, fo) != 1U) {
                fprintf(stderr, "write error\n");
                fclose(fo);
                fclose(fi);
                break;
            }

            i++; /* go to the next chunk */

            continue;
        } else if (feof(fi)) {
            /* end of file */

            if (rs != 0U && fwrite(buf, rs, 1U, fo) != 1U) {
                fprintf(stderr, "write error\n");
            }

            fclose(fo);
            fclose(fi);

            fprintf(stdout, "finished\n");

            break;
        } else {
            /* read error */

            /* ask the user what should we do */
            fprintf(stderr, "error when reading from byte %zu\n",
                    i * chunk + start);
            if (!is_always_z) {
                fprintf(stdout, "Retry? "
                        "Please enter \"y\" to retry (default), "
                        "\"n\" to fill with zeros, or "
                        "\"z\" to always fill with zeros [Ynz]: ");
                if (fgets(user_input, sizeof user_input,
                          stdin) == NULL) {
                    fprintf(stderr, "can not read user input\n");
                    fclose(fo);
                    break;
                }

                if (user_input[0U] == 'Z' || user_input[0U] == 'z') {
                    is_always_z = true;
                }
            }

            /* fill the chunk with 0s or retry, based on the answer */
            if (is_always_z ||
                    user_input[0U] == 'N' || user_input[0U] == 'n') {
                memset(buf, 0, chunk);
                if (fwrite(buf, chunk, 1U, fo) != 1U) {
                    fprintf(stderr, "write error\n");
                    fclose(fo);
                    fclose(fi);
                    break;
                }

                i++; /* go to the next chunk */
            } else {
                fprintf(stdout, "retrying...\n");
            }

            /* will reopen the input file on the next loop iteration */
            fclose(fi);
            fi = NULL;

            continue;
        }
    }

    free(buf);

    return EXIT_SUCCESS;
}
