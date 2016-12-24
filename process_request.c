#include "request.h"  /* parse_request() */
#include "helpers.h" /* vprintf() */
#include <stdlib.h>  /* exit() and EXIT_FAILURE */
#include <stdio.h>  /* printf() */

/* 处理http请求 */
void process_request(int conn_fd) {
    request req;

    vprintf("初始化请求\n");
    init_request(&req);

    if (parse_request(conn_fd, &req) == -1) {
        perror("parse_request");
        exit(EXIT_FAILURE);
    }
    dispatch(conn_fd, req);
}
