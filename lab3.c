#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Глобальные переменные
int shared_resource = 0;
pthread_mutex_t mutex;
sem_t semaphore;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция для мьютекса
void *mutex_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    shared_resource++;
    printf("Мьютекс: shared_resource = %d\n", shared_resource);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// Функция для семафора
void *semaphore_thread(void *arg) {
    sem_wait(&semaphore);
    shared_resource++;
    printf("Семафор: shared_resource = %d\n", shared_resource);
    sem_post(&semaphore);
    return NULL;
}

// Функция для условной переменной
void *cond_var_thread(void *arg) {
    pthread_mutex_lock(&cond_mutex);
    while (shared_resource < 5) {
        pthread_cond_wait(&cond_var, &cond_mutex);
    }
    printf("Условная переменная: shared_resource >= 5\n");
    pthread_mutex_unlock(&cond_mutex);
    return NULL;
}

int main() {
    pthread_t threads[3];

    // Инициализация
    pthread_mutex_init(&mutex, NULL);
    sem_init(&semaphore, 0, 1);

    // Создание потоков
    pthread_create(&threads[0], NULL, mutex_thread, NULL);
    pthread_create(&threads[1], NULL, mutex_thread, NULL);
    pthread_create(&threads[2], NULL, semaphore_thread, NULL);

    pthread_t cond_thread;
    pthread_create(&cond_thread, NULL, cond_var_thread, NULL);

    // Изменение ресурса для условной переменной
    sleep(1);
    pthread_mutex_lock(&cond_mutex);
    shared_resource = 10;
    pthread_cond_signal(&cond_var);
    pthread_mutex_unlock(&cond_mutex);

    // Ожидание завершения
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(cond_thread, NULL);

    // Очистка
    pthread_mutex_destroy(&mutex);
    sem_destroy(&semaphore);

    return 0;
}
