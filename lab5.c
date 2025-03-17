#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define MODE_EQUAL 1
#define MODE_MORE_ALLOC 2

int mode;
void *ptrs[1000];
int count = 0;

void sigint_handler(int sig) {
    for (int i = 0; i < count; i++) {
        free(ptrs[i]);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <mode>\n", argv[0]);
        return 1;
    }
    mode = atoi(argv[1]);
    signal(SIGINT, sigint_handler);

    while (1) {
        if (mode == MODE_EQUAL) {
            void *ptr = malloc(1024);
            if (ptr) {
                ptrs[count++] = ptr;
                free(ptr);
                count--;
            }
        } else if (mode == MODE_MORE_ALLOC) {
            void *ptr = malloc(1024);
            if (ptr) {
                ptrs[count++] = ptr;
            }
        }
        usleep(100000); // 100 мс
    }

    return 0;
}
