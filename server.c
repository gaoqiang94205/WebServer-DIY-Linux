#include "process_request.h"  /* 处理请求 */
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#define PORT "8080" //监听的端口号
#define BACKLOG 10  // 服务器队列等待的连接数

int verbose; // 是否后台显示详细信息
char *root_path;  // web目录根路径

void sigchld_handler(int s) {
    // pid>0 等待任何子进程识别码为pid 的子进程
    // pid_t waitpid(pid_t pid, int * status, int options);
    //不在意结束状态值, 则参数status 可以设成NULL
    // 等待任何子进程     whohang 如果没有任何已经结束的子进程则马上返回, 不予以等待.
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// returns ptr to in_addr (IPv4) or in6_addr (IPv6) struct for inet_ntop()
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

//输入参数为web目录的根路径
int main(int argc, char **argv) {
    int listen_fd;
    int new_fd;
    int status;
    char *options = "v"; //选项参数
    int reuse_val = 1;
    char in_addr[INET6_ADDRSTRLEN];
    struct sigaction sa;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;
    struct sockaddr_storage cli_addr;
    socklen_t cli_addr_sz = sizeof cli_addr;
    char c;
    verbose = 1;

    //相对路径转换成绝对路径后存于 root_path
    if (optind < argc) {
        if ((root_path = realpath(argv[optind], root_path)) == NULL) {
            fprintf(stderr, "%s: 路径 error\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "用法: %s [%s] <root path>\n", argv[0], options);
        exit(EXIT_FAILURE);
    }

    //初始化hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //表示IPv4 IPv6均可
    hints.ai_socktype = SOCK_STREAM; //流式socket
    hints.ai_flags = AI_PASSIVE;

    //处理名字到地址以及服务到端口这两 种转换
    //返回0：  成功 返回非0：  出错
    //如果本函数返回成功，那么由result参数指向的变量已被填入一个指针，它指向的是由其中的ai_next成员串联起来的addrinfo结构链表
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    /* 创建 socket socket()用于创建一个socket描述符（socket descriptor）
     * 它唯一标识一个socket。这个socket描述字跟文件描述字一样，后续的操作都有用到它，把它作为参数，通过它来进行一些读写操作 */
    for (p = res; p != NULL; p = p->ai_next) {
        if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        /* 如果地址已经在使用就报错 */
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                       &reuse_val, sizeof(int)) == -1) {
            perror("server: setsockopt");
            exit(1);
        }

        /* bind()函数把一个地址族中的特定地址赋给socket。例如对应AF_INET、AF_INET6就是把一个ipv4或ipv6地址和端口号组合赋给socket */
        if (bind(listen_fd, res->ai_addr, res->ai_addrlen) == -1) {
            close(listen_fd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(res);

    /* 如果作为一个服务器，在调用socket()、bind()之后就会调用listen()来监听这个socket，如果客户端这时调用connect()发出连接请求，服务器端就会接收到这个请求。 */
    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    /* 设置主进程忽略子进程死亡信号,因此不需要为每一个子进程阻塞 */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("server: 信号错误");
        exit(1);
    }

    printf("等待连接...\n");

    /* 接收客户端发来的连接 */
    while (1) {
        /* 等待连接 返回值是由内核自动生成的一个全新的描述字，代表与返回客户的TCP连接 */
        new_fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &cli_addr_sz);
        if (new_fd == -1) {
            perror("server: accept");
            continue;
        }

        /* 将获取到的连接转换成可以打印的格式*/
        inet_ntop(cli_addr.ss_family, get_in_addr((struct sockaddr *) &cli_addr),
                  in_addr, sizeof in_addr);
        printf("连接来自: %s\n", in_addr);

        /* 开启子进程去处理连接的请求 */
        if (fork() == 0) {

            /* 这是分叉的子进程,关闭监听套接字和处理请求 */

            if (close(listen_fd) == -1) {
                perror("server: 关闭错误111");
                exit(EXIT_FAILURE);
            }

            process_request(new_fd);

            vprintf("子进程退出...\n");
            /* 关闭 */
            if (close(new_fd) == -1) {
                perror("server: 关闭错误222");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }

        /* 父进程关闭 */
        if (close(new_fd) == -1) {
            perror("server: 关闭父进程中的连接出错");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
