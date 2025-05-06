#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

void* connect_to_server(void *port_ptr) {
    int port = *((int*)port_ptr);
    const char *ip = gtk_entry_get_text(GTK_ENTRY(entry_ip));
    if (strlen(ip) == 0) {
	ip = "127.0.0.1";
    }

    if (inet_addr(ip) == INADDR_NONE) {
        append_text("Неверный IP-адрес!\n");
        return NULL;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;           // Исправлено
    addr.sin_port = htons(port);         // Исправлено
    addr.sin_addr.s_addr = inet_addr(ip);// Исправл0ено
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        append_text("Ошибка создания сокета!\n");
        return NULL;
    }
    printf("Connection to %s:%d...\n", ip, port);
    // Исправленный вызов connect
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
	printf("Failed to connect to %s:%d...\n", ip, port);
	perror("Connection error");
	append_text("Ошибка подключения!\n");
        close(sockfd);
        return NULL;
    }
    printf("Connected to to %s:%d...\n", ip, port);
    char buffer[1024];
    ssize_t bytes = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        append_text(buffer);
        append_text("\n---\n");
    }
    append_text("Succes!");
    close(sockfd);
    return NULL;
}


void on_connect(GtkWidget *widget, gpointer data) {
    int port = GPOINTER_TO_INT(data);
    pthread_t thread;
    pthread_create(&thread, NULL, connect_to_server, &port);
    pthread_detach(thread);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    entry_ip = gtk_entry_new();
    GtkWidget *btn_server1 = gtk_button_new_with_label("Сервер 1 (PID)");
    GtkWidget *btn_server2 = gtk_button_new_with_label("Сервер 2 (ОС)");
    text_view = gtk_text_view_new(); // Возвращает GtkWidget*
    
    static int server1_port = SERVER1_PORT; // 8080
    static int server2_port = SERVER2_PORT; // 8081

    g_signal_connect(btn_server1, "clicked", G_CALLBACK(on_connect), &server1_port);
    g_signal_connect(btn_server2, "clicked", G_CALLBACK(on_connect), &server2_port);
    
    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_box_pack_start(GTK_BOX(box), entry_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_server1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_server2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), text_view, TRUE, TRUE, 0);
    
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
