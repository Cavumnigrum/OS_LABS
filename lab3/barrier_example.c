#include <pthread.h>  // для потоков и барьеров
#include <stdio.h>    // для printf()
#include <unistd.h>   // можно оставить, но мы не используем sleep()

// Объект барьера, синхронизирующий 3 потока
pthread_barrier_t barrier;

// Функция, выполняемая каждым потоком
void* task(void* arg) {
    int id = *(int*)arg;  // получаем ID потока

    // Первая фаза — выполняется независимо каждым потоком
    printf("Thread %d: Phase 1 complete\n", id);

    // Барьер — поток здесь блокируется, пока ВСЕ потоки не дойдут до этой точки
    pthread_barrier_wait(&barrier);

    // После того как все потоки дошли до барьера — они продолжают выполнение
    printf("Thread %d: Phase 2 started\n", id);

    return NULL;  // завершение потока
}

int main() {
    pthread_t threads[3];      // массив для 3 потоков
    int ids[3] = {0, 1, 2};    // ID-шники потоков

    // Инициализируем барьер на 3 потока
    // Пока 3 потока не вызовут pthread_barrier_wait — никто не продолжит выполнение
    pthread_barrier_init(&barrier, NULL, 3);

    // Создаём 3 потока
    for (int i = 0; i < 3; ++i)
        pthread_create(&threads[i], NULL, task, &ids[i]);

    // Ждём завершения всех потоков
    for (int i = 0; i < 3; ++i)
        pthread_join(threads[i], NULL);

    // Уничтожаем барьер (все потоки уже завершились)
    pthread_barrier_destroy(&barrier);

    return 0;
}
