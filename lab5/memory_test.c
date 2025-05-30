#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

bool running = true;

void handle_signal(int sig) {
    running = false;
}

int main(int argc, char *argv[]) {
    printf("PID: %d\n", getpid());

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <equal|leak>\n", argv[0]);
        return 1;
    }

    int max_alloc = 1000000;
    void *allocated[max_alloc];

    // Определяем режим работы
    int mode = (strcmp(argv[1], "leak") == 0) ? 1 : 0; 
    // 0 - equal (malloc = free)
    // 1 - leak (malloc > free)

    signal(SIGINT, handle_signal);

    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 100; // 100 нс

    int cur = 0;
    bool clean = false;

    while (running) {
        void *ptr = malloc(1024); // Выделяем 1 КБ
        if (ptr != NULL && cur < max_alloc) {
            allocated[cur++] = ptr;
        }

        if ((mode == 0 || clean) && cur > 0) {
            free(allocated[--cur]);
        }

        clean = !clean; // через раз освобождаем память в режиме leak

        nanosleep(&sleep_time, NULL);
    }

    // Освобождаем всю память при завершении
    for (int i = 0; i < cur; ++i) {
        free(allocated[i]);
    }

    return 0;
}
