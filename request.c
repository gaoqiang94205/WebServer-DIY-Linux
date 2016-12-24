#include "request.h"  /* MAX_REQ_LINE_LENGTH  constant*/
#include "helpers.h"  /* readline(), strip() helpers */
#include "dispatch.h"

#include <stdio.h>  /* printf() */
#include <stdlib.h>  /* exit(), malloc(), calloc() */
#include <string.h>  /* strlen(), strchr() */
#include <unistd.h>  /* ssize_t data type */
#include <sys/select.h>  /* select() */

//解析请求
int parse_req_line(char *buffer, request *req) {
    char *HTTP1_0 = "HTTP/1.0";
    char *HTTP1_1 = "HTTP/1.1";
    int i, len;
    char *offset;

    /* 解析 http 方法 看是get 还是 post */
    len = strchr(buffer, ' ') - buffer;
    for (i = 0; i < module_count; i++) {
        if (!strncmp(module_names[i], buffer, len)) {
            req->method_index = i;
            break;
        }
    }
    if (i == module_count) {
        req->method_index = 2;
    }

    offset = buffer + len;
    /* 定位到URI */
    while (*offset && isspace(*offset)) {
        offset++;
    }

    // could be null
    len = strchr(offset, ' ') - offset;
    req->URI = calloc(len + 1, sizeof(char));
    strncpy(req->URI, offset, len);

    if (strstr(buffer, HTTP1_1)) {
        strncpy(req->version, HTTP1_1, strlen(HTTP1_1));
    } else {
        strncpy(req->version, HTTP1_0, strlen(HTTP1_0));
    }
}

//解析请求头
void parse_header_line(char *buffer, request *req) {
    int len;
    char *offset, *header;

    offset = strchr(buffer, ':');
    if (offset == NULL) {
        req->status = 400;
        req->done = 1;
        return;
    }

    len = offset - buffer;
    header = calloc(len + 1, sizeof(char));
    strncpy(header, buffer, len);
    upcase(header);

    vprintf("header: %s\n", header);

    offset++;
    while (*offset && isspace(*offset)) {
        offset++;
    }
    /* 不存在, 则为 bad request */
    if (*offset == '\0') {
        req->status = 400;
        req->done = 1;
        return;
    }

    vprintf("offset: %s\n", offset);

    if (!strcmp(header, "HOST")) {
        req->host = malloc(strlen(offset) + 1);
        strcpy(req->host, offset);
    } else if (!strcmp(header, "CONTENT-LENGTH")) {
        req->content_length = atoi(offset);
    } else if (!strcmp(header, "USER-AGENT")) {
        req->user_agent = malloc(strlen(offset) + 1);
        strcpy(req->user_agent, offset);
    }

    free(header);
}

//解析 http 请求的 body
void parse_req_body(char *buffer, request *req) {
    req->body = buffer;
    req->done = 1;
}

//解析请求
int parse_request(int conn_fd, request *req) {
    char buffer[MAX_REQ_LINE_LENGTH];
    int first_line = 1, rv, req_has_body = 0;
    fd_set readfds;
    struct timeval tv;


    /* 设置五秒的请求时间 */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    do {
        /* 清除重置文件描述符集 */
        FD_ZERO(&readfds);
        FD_SET(conn_fd, &readfds);

        rv = select(conn_fd + 1, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (rv == 0) {
            /* 超时 */
            return -1;
        } else {
            readline(conn_fd, buffer, MAX_REQ_LINE_LENGTH - 1);
            strip(buffer);
            vprintf("buffer [%3zu]: %s\n", strlen(buffer), buffer);

            if (first_line) {
                parse_req_line(buffer, req);
                first_line--;
            } else if (!strncmp(buffer, "\r\n", 2)) {
                /* 空行的情况 */
                vprintf("Blank line!\n");
                /* 检查收到的请求头里面时候有content_length */
                if (req->content_length <= 0) {
                    req->done = 1;
                } else {
                    req_has_body = 1;
                    req->done = 1;
                }
            } else if (req_has_body) {
                parse_req_body(buffer, req);
            } else {
                parse_header_line(buffer, req);
            }
        }
    } while (req->done == 0);
    if (req->content_length > 0) {
        req->content_length = read_body(conn_fd, &(req->body), req->content_length);
    } else {
        req->body = NULL;
    }
}

//初始化请求
void init_request(request *req) {
    //访问结构体成员
    req->method_index = -1;
    req->status = 200;
    req->URI = NULL;
    req->host = NULL;
    req->content_length = -1;
    req->user_agent = NULL;
    req->done = 0;
}
