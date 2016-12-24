/*

  request.h
  =========
    
  Authors: Kyle Poore, Robert Correiro
 
  Implementation of functions to parse HTTP request headers.
 
*/

#ifndef REQUEST_H
#define REQUEST_H

#define METHOD_NAME_LENGTH  8
#define HTTP_VERSION_LENGTH 9

#define MAX_REQ_LINE_LENGTH 1024

//封装请求的信息到结构体中
typedef struct request {
  int method_index;
  int status;
  char *URI;
  char version[HTTP_VERSION_LENGTH];
  char *host;
  int content_length;
  char *user_agent;
  int done;
  char *body;
} request;

int parse_req_line(char *buffer, request *req);
void parse_header_line(char *buffer, request *req);
int parse_request(int conn_fd, request *req);

#endif //REQUEST_H
