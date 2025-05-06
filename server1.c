#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

// Проверка на повторный запуск
void check_singleton() {
    int lock_fd = open("/tmp/server1.lock", O_CREAT | O_RDWR, 0666);
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
        fprintf(stderr, "Сервер уже запущен!\n");
        exit(EXIT_FAILURE);
    }
}

void* handle_client(void* arg) {
    int sockfd = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    pid_t pid = getpid();
    int priority = getpriority(PRIO_PROCESS, 0);

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    snprintf(buffer, BUFFER_SIZE, 
        "Server PID: %d\nPriority: %d\nTime: %s", 
        pid, priority, timestamp
    );
    
    send(sockfd, buffer, strlen(buffer)+1, 0);
    close(sockfd);
    return NULL;
}

int main() {
    check_singleton(); // Блокировка повторного запуска
    
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, MAX_CLIENTS);

    printf("Сервер 1 запущен на порту %d\n", PORT);
    
    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        int *arg = malloc(sizeof(int));
        *arg = client_fd;
        
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, arg);
        pthread_detach(thread);
    }
}
