// Подключение всех необходимых библиотек
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>         // fork, pipe, write, read
#include <fcntl.h>          // для open и fcntl
#include <sys/mman.h>       // разделяемая память
#include <sys/socket.h>     // сокеты
#include <sys/un.h>         // UNIX domain sockets
#include <string.h>         // memcpy, strncpy
#include <sys/wait.h>       // ожидание процессов
#include <semaphore.h>      // POSIX семафоры
#include <errno.h>

#define SHM_NAME "/det_shm"              // имя разделяемой памяти
#define SOCK_PATH "det_socket"           // имя UNIX-сокета
#define SEM_NAME "/SEM_SOCKET_READY"     // имя семафора
#define MAX 10                           // максимальный размер матрицы

// Цвета для удобного отображения логов
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define PURPLE  "\033[35m"
#define CYAN    "\033[36m"

// Структура, с помощью которой передаётся минор матрицы
typedef struct {
    int size;              // размер минора (n - 1)
    int data[MAX][MAX];    // содержимое минора
} MinorPacket;

// Получение минора: удаляем 0-ю строку и указанный столбец
void get_minor_matrix(int n, int src[MAX][MAX], int exclude_col, int dest[MAX][MAX]) {
    for (int i = 1, dst_i = 0; i < n; i++, dst_i++) {
        int dst_j = 0;
        for (int j = 0; j < n; j++) {
            if (j == exclude_col) continue;
            dest[dst_i][dst_j++] = src[i][j];
        }
    }
}

// Рекурсивная функция вычисления определителя матрицы
int recursive_determinant(int n, int mat[MAX][MAX]) {
    if (n == 1) return mat[0][0];
    if (n == 2) return mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0];

    int det = 0;
    int sign = 1;
    int minor[MAX][MAX];

    for (int col = 0; col < n; col++) {
        get_minor_matrix(n, mat, col, minor);
        det += sign * mat[0][col] * recursive_determinant(n - 1, minor);
        sign *= -1; // чередование знаков
    }

    return det;
}

// Процесс-воркер, который получает задачу по PIPE и возвращает результат по другому PIPE
void worker_pipe(int pipefd[], int result_fd) {
    close(pipefd[1]);  // закрываем запись

    MinorPacket packet;
    read(pipefd[0], &packet, sizeof(MinorPacket));
    close(pipefd[0]);

    int det = recursive_determinant(packet.size, packet.data);
    printf(BLUE "[PIPE WORKER] Минор обработан. Определитель = %d\n" RESET, det);

    write(result_fd, &det, sizeof(int));  // отправляем результат в канал
    close(result_fd);
    exit(0);
}

// Воркер, который принимает задачу через SOCKET и возвращает результат через SHM
void worker_socket() {
    sem_t* sem = sem_open(SEM_NAME, 0);  // открываем уже созданный семафор

    // Настраиваем сокет
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCK_PATH); // на всякий случай удалим старый сокет

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror(RED "[SOCKET WORKER] Ошибка bind" RESET);
        exit(1);
    }

    sem_post(sem); // говорим мастеру, что сокет готов принимать
    sem_close(sem);

    MinorPacket packet;
    ssize_t recv_bytes = recvfrom(sockfd, &packet, sizeof(MinorPacket), 0, NULL, NULL);
    if (recv_bytes <= 0) {
        perror(RED "[SOCKET WORKER] Ошибка recvfrom" RESET);
        exit(1);
    }

    int det = recursive_determinant(packet.size, packet.data);
    printf(PURPLE "[SOCKET WORKER] Минор обработан. Определитель = %d\n" RESET, det);

    // Запись результата в shm
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    int* shm_ptr = mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *shm_ptr = det;

    close(shm_fd);
    close(sockfd);
    unlink(SOCK_PATH);
    exit(0);
}
// Главный процесс
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

    // Очистка ресурсов
    unlink(SOCK_PATH);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);

    // PIPE для передачи задачи
    int pipefd[2];
    pipe(pipefd);

    // PIPE для возврата результата
    int result_pipe[2];
    pipe(result_pipe);

    // Создание разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int));
    int* shm_ptr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Создание POSIX семафора
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);

    // Сокет для отправки миноров
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sun_family = AF_UNIX;
    strncpy(dest_addr.sun_path, SOCK_PATH, sizeof(dest_addr.sun_path) - 1);

    // Запускаем процессы
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(result_pipe[0]); // закрыть чтение
        worker_pipe(pipefd, result_pipe[1]);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        worker_socket();
    }

    // MINOR[0] — передаётся по PIPE
    MinorPacket packet1;
    packet1.size = n - 1;
    get_minor_matrix(n, matrix, 0, packet1.data);
    close(pipefd[0]);  // закрываем чтение
    printf(CYAN "[MASTER] Отправляю MINOR[0] через PIPE\n" RESET);
    write(pipefd[1], &packet1, sizeof(MinorPacket));
    close(pipefd[1]);  // закрываем запись

    // MINOR[1] — передаётся по SOCKET
    MinorPacket packet2;
    packet2.size = n - 1;
    get_minor_matrix(n, matrix, 1, packet2.data);
    printf(CYAN "[MASTER] Жду готовности сокета...\n" RESET);
    sem_wait(sem); // ждём готовности сокет-процесса
    printf(CYAN "[MASTER] Отправляю MINOR[1] через SOCKET\n" RESET);
    sendto(sockfd, &packet2, sizeof(MinorPacket), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    close(sockfd);  // закрываем сокет
    sem_close(sem);
    sem_unlink(SEM_NAME);

    // Получение результата от PIPE
    int det = 0, sign = 1;
    int d1;
    read(result_pipe[0], &d1, sizeof(int));
    close(result_pipe[0]);
    waitpid(pid1, NULL, 0);
    det += sign * matrix[0][0] * d1;
    sign *= -1;

    // Получение результата из shm
    waitpid(pid2, NULL, 0);
    int d2 = *shm_ptr;
    det += sign * matrix[0][1] * d2;
    sign *= -1;

    // Остальные миноры считаются локально
    for (int i = 2; i < n; i++) {
        MinorPacket local;
        local.size = n - 1;
        get_minor_matrix(n, matrix, i, local.data);
        int d = recursive_determinant(local.size, local.data);
        printf(YELLOW "[MASTER] MINOR[%d] обработан локально. Определитель = %d\n" RESET, i, d);
        det += sign * matrix[0][i] * d;
        sign *= -1;
    }

    // Вывод финального результата
    printf(GREEN "\n[MASTER] Финальный определитель матрицы: %d\n" RESET, det);

    shm_unlink(SHM_NAME);
    return 0;
}
