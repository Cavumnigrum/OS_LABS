#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>

#define SHM_NAME "/det_shm"
#define SOCK_PATH "det_socket"
#define SEM_NAME "/SEM_SOCKET_READY"
#define MAX 10

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define PURPLE  "\033[35m"
#define CYAN    "\033[36m"

typedef struct {
    int size;
    int data[MAX][MAX];
} MinorPacket;

void get_minor_matrix(int n, int src[MAX][MAX], int exclude_col, int dest[MAX][MAX]) {
    for (int i = 1, dst_i = 0; i < n; i++, dst_i++) {
        int dst_j = 0;
        for (int j = 0; j < n; j++) {
            if (j == exclude_col) continue;
            dest[dst_i][dst_j++] = src[i][j];
        }
    }
}

int recursive_determinant(int n, int mat[MAX][MAX]) {
    if (n == 1) return mat[0][0];
    if (n == 2)
        return mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0];
    int det = 0, sign = 1, minor[MAX][MAX];
    for (int col = 0; col < n; col++) {
        get_minor_matrix(n, mat, col, minor);
        det += sign * mat[0][col] * recursive_determinant(n - 1, minor);
        sign *= -1;
    }
    return det;
}

void worker_pipe(int pipefd[], int result_fd) {
    close(pipefd[1]);
    MinorPacket packet;
    read(pipefd[0], &packet, sizeof(MinorPacket));
    close(pipefd[0]);

    int det = recursive_determinant(packet.size, packet.data);
    printf(BLUE "[PIPE WORKER] Минор обработан. Определитель = %d\n" RESET, det);

    write(result_fd, &det, sizeof(int));
    close(result_fd);
    exit(0);
}

void worker_socket() {
    sem_t* sem = sem_open(SEM_NAME, 0);

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCK_PATH);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror(RED "[SOCKET WORKER] Ошибка bind" RESET);
        exit(1);
    }

    sem_post(sem);
    sem_close(sem);

    MinorPacket packet;
    ssize_t recv_bytes = recvfrom(sockfd, &packet, sizeof(MinorPacket), 0, NULL, NULL);
    if (recv_bytes <= 0) {
        perror(RED "[SOCKET WORKER] Ошибка recvfrom" RESET);
        exit(1);
    }

    int det = recursive_determinant(packet.size, packet.data);
    printf(PURPLE "[SOCKET WORKER] Минор обработан. Определитель = %d\n" RESET, det);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    int* shm_ptr = mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *shm_ptr = det;

    close(shm_fd);
    close(sockfd);
    unlink(SOCK_PATH);
    exit(0);
}

int main() {
    int n, matrix[MAX][MAX];
    printf(CYAN "Введите размер матрицы (2-%d): " RESET, MAX);
    scanf("%d", &n);
    if (n < 2 || n > MAX) {
        printf(RED "Неверный размер\n" RESET);
        return 1;
    }

    printf(CYAN "Введите элементы матрицы построчно:\n" RESET);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            scanf("%d", &matrix[i][j]);

    // Очистка IPC
    unlink(SOCK_PATH);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);

    int pipefd[2];
    pipe(pipefd);

    int result_pipe[2];
    pipe(result_pipe);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int));
    int* shm_ptr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sun_family = AF_UNIX;
    strncpy(dest_addr.sun_path, SOCK_PATH, sizeof(dest_addr.sun_path) - 1);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(result_pipe[0]); // читается в main
        worker_pipe(pipefd, result_pipe[1]);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) worker_socket();

    // MINOR[0] → PIPE
    MinorPacket packet1;
    packet1.size = n - 1;
    get_minor_matrix(n, matrix, 0, packet1.data);
    close(pipefd[0]);
    printf(CYAN "[MASTER] Отправляю MINOR[0] через PIPE\n" RESET);
    write(pipefd[1], &packet1, sizeof(MinorPacket));
    close(pipefd[1]);

    // MINOR[1] → SOCKET
    MinorPacket packet2;
    packet2.size = n - 1;
    get_minor_matrix(n, matrix, 1, packet2.data);
    printf(CYAN "[MASTER] Жду готовности сокета...\n" RESET);
    sem_wait(sem);
    printf(CYAN "[MASTER] Отправляю MINOR[1] через SOCKET\n" RESET);
    sendto(sockfd, &packet2, sizeof(MinorPacket), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    close(sockfd);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    // Получение результатов
    int det = 0, sign = 1;

    int d1;
    read(result_pipe[0], &d1, sizeof(int));
    close(result_pipe[0]);
    waitpid(pid1, NULL, 0);
    det += sign * matrix[0][0] * d1;
    sign *= -1;

    waitpid(pid2, NULL, 0);
    int d2 = *shm_ptr;
    det += sign * matrix[0][1] * d2;
    sign *= -1;

    for (int i = 2; i < n; i++) {
        MinorPacket local;
        local.size = n - 1;
        get_minor_matrix(n, matrix, i, local.data);
        int d = recursive_determinant(local.size, local.data);
        printf(YELLOW "[MASTER] MINOR[%d] обработан локально. Определитель = %d\n" RESET, i, d);
        det += sign * matrix[0][i] * d;
        sign *= -1;
    }

    printf(GREEN "\n[MASTER] Финальный определитель матрицы: %d\n" RESET, det);
    shm_unlink(SHM_NAME);
    return 0;
}
