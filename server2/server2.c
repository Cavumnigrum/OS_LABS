#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <fcntl.h>
#include <sys/file.h>

#define PORT 8081
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

void check_singleton() {
    int lock_fd = open("/tmp/server2.lock", O_CREAT | O_RDWR, 0666);
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
        fprintf(stderr, "Сервер уже запущен!\n");
        exit(EXIT_FAILURE);
    }
}

char* get_keyboard_layout(){
	char buffer[32];
	FILE *pipe = popen("xkblayout-state print \"%s\" 2>/dev/null","r");
	if (!pipe) return NULL;
	
	fgets(buffer, sizeof(buffer), pipe);
	pclose(pipe);

	size_t len = strlen(buffer);
	if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
	
	return strdup(buffer);
}
void* handle_client(void* arg) {
    int sockfd = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    char *layout = get_keyboard_layout();
    
    struct utsname os_info;
    uname(&os_info);

    snprintf(buffer, BUFFER_SIZE,
        "Keyboard Layout: %s\nOS Version: %s %s\n",
        layout, os_info.sysname, os_info.release
    );
    
    send(sockfd, buffer, strlen(buffer)+1, 0);
    close(sockfd);
    return NULL;
}

int main() {
    check_singleton();
    
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, MAX_CLIENTS);

    printf("Сервер запущен на порту %d\n", PORT);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        int *arg = malloc(sizeof(int));
        *arg = client_fd;
        
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, arg);
        pthread_detach(thread);
    }
}
