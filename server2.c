#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <locale.h>
#include <langinfo.h>
#include <time.h>

#define PORT 8081 // Порт сервера
#define MAX_CLIENTS 5 // Максимальное число клиентов
#define BUFFER_SIZE 1024 // Размер буфера сообщений

// Функция-обработчик сигнала SIGCHLD для очистки зомби-процессов
void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Структура потока-клиента
typedef struct client_thread_data {
    int socket_fd;
    char *client_ip;
} client_thread_data_t;

// Потоковая функция для обслуживания клиентов
void* handle_client(void* arg) {
    client_thread_data_t *data = (client_thread_data_t*)arg;
    int sockfd = data->socket_fd;
    free(data->client_ip);
    free(data);
    
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_read = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if(bytes_read <= 0){
        close(sockfd);
        return NULL;
    }

    // Устанавливаем локаль и получаем текущую раскладку клавиатуры
    setlocale(LC_ALL, "");
    const char *layout_code = nl_langinfo(CODESET);

    // Определяем версию операционной системы
    FILE *os_version_fp = fopen("/etc/os-release", "r");
    char os_release_line[256], version_str[256];
    strcpy(version_str, "Unknown");
    if(os_version_fp){
        while(fgets(os_release_line, sizeof(os_release_line), os_version_fp)){
            if(strncmp(os_release_line, "VERSION=", 8) == 0){
                sscanf(os_release_line, "VERSION=\"%[^\"\"]\"", version_str);
                break;
            }
        }
        fclose(os_version_fp);
    }

    // Формируем ответ клиенту
    time_t current_time = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(¤t_time));

    snprintf(buffer, sizeof(buffer),
            "Keyboard Layout: %s\nOS Version: %s\nTime: %s",
            layout_code, version_str, timestamp);

    send(sockfd, buffer, strlen(buffer)+1, 0);
    close(sockfd);
    pthread_exit(NULL);
}

// Основной цикл сервера
int main() {
    // Установка обработчика сигнала SIGCHLD
    signal(SIGCHLD, sigchld_handler);

    // Создаем TCP/IP сокет
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    // Определяем адрес прослушивания
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Привязываем сокет к адресу
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0){
        perror("Ошибка привязки адреса");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Начинаем прослушивание входящих соединений
    if(listen(listenfd, MAX_CLIENTS) != 0){
        perror("Ошибка при попытке начать прослушивание");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен и ожидает соединения...\n");

    fd_set master_set, readfds;
    FD_ZERO(&master_set);
    FD_SET(listenfd, &master_set);
    int maxfd = listenfd + 1;

    while(1){
        readfds = master_set;
        select(maxfd, &readfds, NULL, NULL, NULL);
        
        for(int i = 0; i < maxfd; ++i){
            if(FD_ISSET(i, &readfds)){
                if(i == listenfd){ // Новый клиент пытается подключиться
                    struct sockaddr_in cliaddr;
                    socklen_t addrlen = sizeof(cliaddr);
                    int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &addrlen);
                    
                    if(connfd >= 0){
                        // Создаем новый поток для обслуживания нового клиента
                        client_thread_data_t *thread_data = malloc(sizeof(client_thread_data_t));
                        thread_data->socket_fd = connfd;
                        
                        char ip_buf[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_buf, sizeof(ip_buf));
                        thread_data->client_ip = strdup(ip_buf);

                        pthread_t tid;
                        pthread_create(&tid, NULL, handle_client, thread_data);
                        pthread_detach(tid);
                    }
                }
            }
        }
    }

    close(listenfd);
    return EXIT_SUCCESS;
}
