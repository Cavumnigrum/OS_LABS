#include <pthread.h>   // библиотека для работы с потоками (Pthreads)
#include <stdio.h>     // для функций printf()

#define N 100          // максимальный размер массива

int arr[N];            // общий массив для записи значений
int arr_index = 0;         // индекс, указывающий на следующую свободную ячейку
pthread_mutex_t lock;  // мьютекс для синхронизации доступа к массиву

// Функция, выполняемая потоком
void* writer(void* arg) {
    for (int i = 0; i < 25; ++i) {
        pthread_mutex_lock(&lock);   //  блокируем мьютекс — входим в критическую секцию

        // Защищаем участок кода, чтобы только один поток писал в массив
        if (arr_index < N) {
            arr[arr_index++] = i;        // записываем значение и сдвигаем индекс
        }

        pthread_mutex_unlock(&lock); // разблокируем мьютекс — освобождаем критическую секцию
    }

    return NULL; // функция возвращает NULL, так как ничего не возвращает потоку
}

int main() {
    pthread_t threads[4];  // массив для хранения идентификаторов 4 потоков

    pthread_mutex_init(&lock, NULL);  // инициализация мьютекса (без дополнительных атрибутов)

    // Создание 4 потоков, которые будут выполнять функцию writer
    for (int i = 0; i < 4; ++i)
        pthread_create(&threads[i], NULL, writer, NULL);

    // Ожидание завершения всех потоков
    for (int i = 0; i < 4; ++i)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&lock);  // уничтожаем мьютекс после завершения работы

    // Вывод содержимого массива
    for (int i = 0; i < arr_index; ++i)
        printf("%d ", arr[i]);
    printf("\n");

    return 0;
}
