#include <stdio.h>      // Библиотека для стандартного ввода-вывода (printf)
#include <stdlib.h>     // Библиотека для функций управления памятью и завершения процессов (exit)
#include <pthread.h>    // Библиотека для работы с потоками (pthread_create, pthread_join)
#include <unistd.h>     // Библиотека для работы с процессами и потоками (getpid, sleep)
#include <time.h>       // Библиотека для работы со временем (time, localtime, strftime)

// Функция для вывода текущего времени в формате "часы:минуты:секунды"
void print_time() {
    time_t t;
    struct tm *tm_info;
    char time_buffer[9];

    time(&t);                    // Получаем текущее время
    tm_info = localtime(&t);     // Преобразуем его в локальное время
    strftime(time_buffer, 9, "%H:%M:%S", tm_info); // Форматируем время в строку "чч:мм:сс"
    printf("Текущее время: %s\n", time_buffer);  // Выводим время на экран
}

// Функция, которая будет выполняться в потоках
void* thread_function(void* arg) {
    // Выводим информацию о потоке: его ID и PID родительского процесса
    printf("Поток: ID=%lu, Родительский PID=%d\n", pthread_self(), getpid());
    
    // Выводим текущее время
    print_time();

    // Завершаем выполнение потока
    pthread_exit(NULL);
}

int main() {
    pthread_t thread1, thread2;  // Объявляем два потока

    // Выводим информацию о родительском процессе
    printf("Родительский процесс: PID=%d, PPID=%d\n", getpid(), getppid());
    print_time();  // Выводим текущее время

    // Создаём первый поток, который будет выполнять thread_function()
    pthread_create(&thread1, NULL, thread_function, NULL);

    // Создаём второй поток, который также будет выполнять thread_function()
    pthread_create(&thread2, NULL, thread_function, NULL);

    // Ожидаем завершения первого потока
    pthread_join(thread1, NULL);

    // Ожидаем завершения второго потока
    pthread_join(thread2, NULL);

    return 0;  // Завершаем выполнение программы
}
