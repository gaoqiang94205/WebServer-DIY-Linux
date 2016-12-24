#include <unistd.h>
#include <stdio.h>
// 对我们的服务器接口来辅助函数

#define vprintf(format, ...) do {            \
  if (verbose)                               \
    fprintf(stderr, format, ##__VA_ARGS__);  \
} while (0);

extern int verbose;
extern char *root_path;

ssize_t readline(int sockfd, void *vptr, size_t maxlen);

void strip(char *buffer);
