#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int main() {
    pid_t pid1, pid2;

    // Первый дочерний процесс
    pid1 = fork();
    if (pid1 == 0) {
        printf("Дочерний процесс 1: PID=%d, PPID=%d\n", getpid(), getppid());
        time_t now = time(NULL);
        struct tm *local_time = localtime(&now);
        printf("Время: %02d:%02d:%02d\n", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
        system("ip route"); // Вариант 1: таблица маршрутизации
        exit(0);
    } else if (pid1 > 0) {
        // Родительский процесс
        printf("Родительский процесс: PID=%d\n", getpid());
        time_t now = time(NULL);
        struct tm *local_time = localtime(&now);
        printf("Время: %02d:%02d:%02d\n", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

        // Второй дочерний процесс
        pid2 = fork();
        if (pid2 == 0) {
            printf("Дочерний процесс 2: PID=%d, PPID=%d\n", getpid(), getppid());
            now = time(NULL);
            local_time = localtime(&now);
            printf("Время: %02d:%02d:%02d\n", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
            execlp("./threads", "threads", NULL); // Замещение процессом с потоками
            exit(0);
        } else if (pid2 > 0) {
            system("ps -x"); // Вывод списка процессов
            wait(NULL);
            wait(NULL);
        }
    }

    return 0;
}
