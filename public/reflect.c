#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    char buffer[256 + 1];
    memset(buffer, '\0', sizeof(buffer));
    int bytes = 0;
    while (fgets(buffer, 255, stdin) != NULL) {
        bytes += strlen(buffer);
        buffer[strlen(buffer)] = 0;
        fprintf(stderr, "%s\n", buffer);
        printf("reflect: %s\n", buffer);
    }
    return 0;
}
