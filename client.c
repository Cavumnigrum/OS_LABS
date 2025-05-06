#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <gtk/gtk.h>

#define SERVER1_PORT 8080
#define SERVER2_PORT 8081
#define BUFFER_SIZE 1024

GtkWidget *window;
GtkTextView *text_view;

static void on_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

// Вспомогательная функция для вывода текста в текстовом виджете
void append_text(const char *msg) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    gtk_text_buffer_insert_at_cursor(buffer, msg, -1);
}

// Параметры сервера
struct server_params {
    char *hostname;
    unsigned short port;
};

// Флаг для прекращения циклического опроса
volatile sig_atomic_t stop_flag = 0;

// Сигнал для выхода из цикла
void sigint_handler(int signum) {
    stop_flag = 1;
}

// Функция-поток для общения с одним сервером
void* connect_to_server(void* param) {
    struct server_params *params = (struct server_params *)param;
    char hostname[100];
    strcpy(hostname, params->hostname);
    unsigned short port = params->port;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Ошибка открытия сокета.\n");
        pthread_exit(NULL);
    }

    struct hostent *server = gethostbyname(hostname);
    if (!server) {
        fprintf(stderr, "Ошибка разрешения имени хоста.\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Ошибка подключения к серверу.\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    size_t num_bytes = recv(sockfd, buffer, BUFFER_SIZE-1, 0);
    if(num_bytes > 0) {
        printf("\nДанные от сервера (%u):\n%s\n", port, buffer);
    } else {
        fprintf(stderr, "Ошибка приема данных от сервера.\n");
    }

    close(sockfd);
    pthread_exit(NULL);
}

// Основная программа
int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr,"Использование: %s <IP-хост>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *hostname = argv[1]; // IP-адрес или доменное имя сервера

    // Структуры для хранения параметров серверов
    struct server_params servers[] = {
        { .hostname = hostname, .port = SERVER1_PORT },
        { .hostname = hostname, .port = SERVER2_PORT }
    };

    // Массив для управления потоками
    pthread_t threads[sizeof(servers)/sizeof(struct server_params)];
  
    // Инициализация графического интерфейса
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(text_view));
    gtk_widget_show_all(window);

    g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);
    // Регистрация обработчика сигнала Ctrl+C
    signal(SIGINT, sigint_handler);

    // Цикл опроса
    while(!stop_flag) {
        sleep(5); // Пауза между опросами (можно настроить)

        // Повторяем тот же цикл для каждого сервера
        for(size_t i = 0; i < sizeof(threads)/sizeof(pthread_t); i++) {
            pthread_create(&(threads[i]), NULL, connect_to_server, (void*)&servers[i]);
        }

        // Ждём завершения всех потоков
        for(size_t i = 0; i < sizeof(threads)/sizeof(pthread_t); i++) {
            pthread_join(threads[i], NULL);
        }
    }
  
    gtk_main();
    return EXIT_SUCCESS;
}
