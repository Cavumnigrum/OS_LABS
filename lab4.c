#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

// Функция вычисления определителя 2x2
int determinant(int matrix[2][2]) {
    return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
}

int main() {
    int matrix[2][2] = {{1, 2}, {3, 4}};

    // 1. Каналы
    int pipefd[2];
    pipe(pipefd);
    pid_t pid1 = fork();
    if (pid1 == 0) {
        int det = determinant(matrix);
        write(pipefd[1], &det, sizeof(det));
        exit(0);
    } else {
        int det;
        read(pipefd[0], &det, sizeof(det));
        printf("Определитель из канала: %d\n", det);
        wait(NULL);
    }

    // 2. Разделяемая память
    int *shared_det = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    pid_t pid2 = fork();
    if (pid2 == 0) {
        *shared_det = determinant(matrix);
        exit(0);
    } else {
        wait(NULL);
        printf("Определитель из разделяемой памяти: %d\n", *shared_det);
        munmap(shared_det, sizeof(int));
    }

    // 3. Сокеты
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/mysocket");
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    pid_t pid3 = fork();
    if (pid3 == 0) {
        int det = determinant(matrix);
        sendto(sockfd, &det, sizeof(det), 0, (struct sockaddr *)&addr, sizeof(addr));
        exit(0);
    } else {
        int det;
        socklen_t len = sizeof(addr);
        recvfrom(sockfd, &det, sizeof(det), 0, (struct sockaddr *)&addr, &len);
        printf("Определитель из сокета: %d\n", det);
        wait(NULL);
        close(sockfd);
        unlink("/tmp/mysocket");
    }

    return 0;
}
