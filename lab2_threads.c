#include <stdio.h>
#include <pthread.h>
#include <time.h>

void *thread_func(void *arg) {
    pthread_t tid = pthread_self();
    pid_t pid = getpid();
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    printf("Поток ID: %lu, PID: %d, Время: %02d:%02d:%02d\n",
           tid, pid, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread_func, NULL);
    pthread_create(&thread2, NULL, thread_func, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}
