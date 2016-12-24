#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "request.h"
#include "modules/GET.h"
#include "modules/POST.h"

int module_count = 2;

char *module_names[] = {"GET",
                        "POST"
};

//获取支持的请求的方法
void (*modules[])(int, request) = {GET,
                                   POST
};

//请求分发
void dispatch(int conn, request req) {
    if (req.method_index < module_count) {
        modules[req.method_index](conn, req);
    } else {
        perror("dispatch: 不支持的 HTTP 方法");
        exit(EXIT_FAILURE);
    }
}