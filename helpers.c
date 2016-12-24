#include "helpers.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* 从socker中读取数据 */
ssize_t readline(int sockfd, void *vptr, size_t maxlen) {
    ssize_t n, chars_read;
    char c, *buffer;
    int cr = 0;
    buffer = vptr;

    for (n = 1; n < maxlen; n++) {
        if ((chars_read = read(sockfd, &c, 1)) == 1) {
            *buffer++ = c;

            if (c == '\r') {
                cr = 1;
                continue;
            }
            if (cr == 1 && c == '\n') {
                /* 检查是否有空行 */
                if (n == 2) {
                    return n;
                } else {
                    break;
                }
            }
            if (c == '\n') {
                break;
            }
            if (cr == 1) {
                cr = 0;
            }
        } else if (chars_read == 0) {
            if (n == 1) {
                return 0;
            } else {
                break;
            }
        } else {
            if (errno == EINTR) {
                continue;
            }
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    /* Null 终止 */
    *buffer = 0;
    return n;
}

int read_body(int sockfd, char **buffer, int length) {
    int chars_read;
    vprintf("reading body\n");
    *buffer = malloc(length * sizeof(char));
    if ((chars_read = read(sockfd, *buffer, length)) < 0) {
        fprintf(stderr, "Error: can't read %d bytes from body\n", length);
        exit(EXIT_FAILURE);
    } else {
        return chars_read;
    }
}

/* 格式化字符串 */
void strip(char *str) {
    int sz = strlen(str) - 1;

    while (!isalnum(str[sz]) && sz >= 0) {
        str[sz--] = '\0';
    }
}

/* 大写 */
void upcase(char *str) {
    while (*str) {
        *str = toupper(*str);
        str++;
    }
}
