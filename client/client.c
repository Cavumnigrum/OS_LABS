#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define SERVER1_PORT 8080
#define SERVER2_PORT 8081

GtkWidget *text_view;
GtkWidget *entry_ip;

// Функция для безопасного вывода текста в основном потоке
gboolean append_text_safe(gpointer data) {
    char *text = (char*)data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_insert_at_cursor(buffer, text, -1);
    free(text); // Освобождаем память
    return G_SOURCE_REMOVE;
}

// Функция для отправки данных в GUI
void append_text(const char *text) {
    char *copy = strdup(text); // Копируем текст для передачи в основной поток
    g_idle_add(append_text_safe, copy); // Используем g_idle_add
}
// Получение текущей раскладки через setxkbmap
char* get_current_layout() {
    char buffer[128];
    FILE *fp = popen("setxkbmap -query | grep layout", "r");
    if (!fp) return NULL;
    
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    
    char *layout = strchr(buffer, ':');
    if (!layout) return NULL;
    
    layout++; // Пропускаем двоеточие
    while (*layout == ' ') layout++; // Удаляем пробелы
    
    char *end = strchr(layout, '\n');
    if (end) *end = '\0';
    
    return strdup(layout);
}

// Обработчик кнопки смены раскладки
void on_switch_layout(GtkWidget *widget, gpointer data) {
    char *current = get_current_layout();
    if (!current) {
        append_text("Ошибка определения раскладки\n");
        return;
    }
    
    if (strcmp(current, "us") == 0) {
        system("setxkbmap ru");
        append_text("Раскладка изменена на: RU\n");
    } else {
        system("setxkbmap us");
        append_text("Раскладка изменена на: US\n");
    }
    
    free(current);
}

void* connect_to_server(void *port_ptr) {
    int port = *((int*)port_ptr);
    const char *host = gtk_entry_get_text(GTK_ENTRY(entry_ip));
    if (strlen(host) == 0) {
        host = (port == SERVER1_PORT) ? "server1" : "server2";
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; //IPv4 \ IPv6
    hints.ai_socktype = SOCK_STREAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        append_text("Ошибка разрешения имени хоста!\n");
        return NULL;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        append_text("Ошибка создания сокета!\n");
        freeaddrinfo(res);
        return NULL;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        append_text("Ошибка подключения!\n");
        close(sockfd);
        freeaddrinfo(res);
        return NULL;
    }

    char buffer[1024];
    ssize_t bytes = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        append_text(buffer);
    }
    append_text("\n---\n");
    close(sockfd);
    freeaddrinfo(res);
    return NULL;
}

void on_connect(GtkWidget *widget, gpointer data) {
    int port = *((int*)data);
    pthread_t thread;
    printf("port: %d\n",port);
    pthread_create(&thread, NULL, connect_to_server, data);
    pthread_detach(thread);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    entry_ip = gtk_entry_new();
    GtkWidget *btn_server1 = gtk_button_new_with_label("Сервер 1 (PID)");
    GtkWidget *btn_server2 = gtk_button_new_with_label("Сервер 2 (ОС)");

    GtkWidget *btn_switch = gtk_button_new_with_label("Смена раскладки");

    text_view = gtk_text_view_new(); // Возвращает GtkWidget*
    
    static int server1_port = SERVER1_PORT; // 8080
    static int server2_port = SERVER2_PORT; // 8081

    g_signal_connect(btn_server1, "clicked", G_CALLBACK(on_connect), &server1_port);
    g_signal_connect(btn_server2, "clicked", G_CALLBACK(on_connect), &server2_port);
    g_signal_connect(btn_switch, "clicked", G_CALLBACK(on_switch_layout), NULL);

    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_box_pack_start(GTK_BOX(box), entry_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_server1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_server2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), text_view, TRUE, TRUE, 0);
    
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
