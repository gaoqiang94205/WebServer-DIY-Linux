#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "GET.h"
#include "../helpers.h"

void GET(int conn, request req) {
    char filename[1024];
    memset(filename, '\0', 1024);
    char *content_type;
    char buffer[1024];
    memset(buffer, '\0', 1024);
    FILE *fp;
    vprintf("GET: %s\n", req.URI);
    if (chdir(root_path)) {
        perror("chdir:");
    }
    char *body = strchr(req.URI, '?');
    int filename_length = body - req.URI;
    if (body != NULL) {
        vprintf("%s\n", body);
    }
    filename_length = (filename_length < 0) ? strlen(req.URI + 1) : filename_length - 1;
    strncpy(filename, req.URI + 1, filename_length);
    if (body == NULL) {
        fp = fopen(filename, "r");
        vprintf("%s/%s\n", root_path, filename);
        if (fp != NULL) {
            //file exists
            req.status = 200;
            fseek(fp, 0, SEEK_END);
            int length = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char *response = "HTTP/1.1 200 OK\r\n";
            vprintf("HTTP/1.1 200 OK\r\n");
            if (!strncmp(strrchr(filename, '.') + 1, "ico", 3)) {
                content_type = "Content-Type: icon\r\n";
                vprintf("Content-Type: icon\r\n");
            } else if (!strncmp(strrchr(filename, '.') + 1, "html", 4)) {
                content_type = "Content-Type: text/html\r\n";
                vprintf("Content-Type: text/html\r\n");
            } else if (!strncmp(strrchr(filename, '.') + 1, "js", 2)) {
                content_type = "Content-Type: text/javascript\r\n";
                vprintf("Content-Type: text/javascript\r\n");
            } else if (!strncmp(strrchr(filename, '.') + 1, "css", 3)) {
                content_type = "Content-Type: text/css\r\n";
                vprintf("Content-Type: text/css\r\n");
            } else if (!strncmp(strrchr(filename, '.') + 1, "pdf", 3)) {
                content_type = "Content-Type: application/pdf\r\n";
                vprintf("Content-Type: application/pdf\r\n");
            } else if (!strncmp(strrchr(filename, '.') + 1, "jpg", 3)) {
                content_type = "Content-Type: image/jpeg\r\n";
                vprintf("Content-Type: image/jpeg\r\n");
            } else if (!strncmp(strrchr(filename, '.') + 1, "png", 3)) {
                content_type = "Content-Type: image/png\r\n";
                vprintf("Content-Type: image/png\r\n");
            } else {
                content_type = "Content-Type: text/plain\r\n";
                vprintf("Content-Type: text/plain\r\n");
            }

            char content_length[32];
            memset(content_length, '\0', 32);
            sprintf(content_length, "Content-Length: %d\r\n\r\n", length);
            vprintf("Content-Length: %d\n", length);
            int bytes_read = 0;

            send(conn, response, strlen(response), 0);
            send(conn, content_type, strlen(content_type), 0);
            send(conn, content_length, strlen(content_length), 0);

            int total_bytes = 0;
            while (1) {
                memset(buffer, '\0', 1024);
                bytes_read = fread(buffer, 1, 1024, fp);
                total_bytes += bytes_read;
                if (!bytes_read) {
                    vprintf("\nno more bytes\n");
                    break;
                }
                vprintf("%s", buffer);
                send(conn, buffer, bytes_read, 0);
                if (bytes_read < 1024) {
                    vprintf("\nno more bytes\n");
                    break;
                }
            }
            vprintf("read/sent %d bytes\n", total_bytes);
            vprintf("sent file!\n");
        } else {
            vprintf("file not found! =)\n");

        }
    } else {
        //fork!
        int fdto[2], fdfrom[2];

        if (pipe(fdfrom) == -1) {
            exit(EXIT_FAILURE);
        }
        if (pipe(fdto) == -1) {
            exit(EXIT_FAILURE);
        }

        int pid;
        char executable[filename_length + 3];
        memset(executable, '\0', filename_length + 3);
        strncpy(executable, "./", 2);
        strncpy(executable + 2, filename, filename_length);
        if ((pid = fork()) < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            //child
            dup2(fdto[0], 0);
            dup2(fdfrom[1], 1);
            close(fdto[1]);
            close(fdfrom[0]);
            execl(executable, executable, NULL);
            perror("exec");
            vprintf("%s\n", executable);
            exit(EXIT_FAILURE);
        } else {
            //parent
            close(fdto[0]);
            close(fdfrom[1]);
            if (write(fdto[1], body, strlen(body)) < strlen(body)) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            close(fdto[1]);
            int bytes;
            char buf[1024];
            char lengthbuf[100];
            char *response = "HTTP/1.1 200 OK\r\n";
            char *content_type = "Content-Type: text/plain\r\n";
            char *transfer_encoding = "Transfer-Encoding: chunked\r\n\r\n";
            send(conn, response, strlen(response), 0);
            vprintf("Send: %s", response);
            send(conn, content_type, strlen(content_type), 0);
            vprintf("Send: %s", content_type);
            send(conn, transfer_encoding, strlen(transfer_encoding), 0);
            vprintf("Send: %s", transfer_encoding);
            memset(buf, '\0', 1024);
            int status, died;
            while ((bytes = read(fdfrom[0], buf, 1024))) {
                if (bytes < 1) {
                    break;
                }
                memset(lengthbuf, '\0', 100);
                sprintf(lengthbuf, "%x\r\n", bytes);
                vprintf("Send: %s", lengthbuf);
                send(conn, lengthbuf, strlen(lengthbuf), 0);
                vprintf("Send: %s", buf);
                send(conn, buf, bytes, 0);
                vprintf("Send: \r\n");
                send(conn, "\r\n", 2, 0);
                memset(buf, '\0', 1024);
            }
            vprintf("Send: 0\r\n\r\n");
            send(conn, "0\r\n\r\n", 5, 0);
            died = wait(&status);
            vprintf("Child exited: %d\n", status);

        }
    }
}
