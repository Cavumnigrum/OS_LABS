#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>

int main() {
    // 1. Получение имени компьютера и пользователя
    char hostname[256];
    char username[256];
    gethostname(hostname, sizeof(hostname));
    getlogin_r(username, sizeof(username));
    printf("Имя компьютера: %s\n", hostname);
    printf("Имя пользователя: %s\n", username);

    // 2. Получение версии операционной системы
    struct utsname uts;
    uname(&uts);
    printf("Операционная система: %s\n", uts.sysname);
    printf("Версия ОС: %s\n", uts.release);
    printf("Архитектура: %s\n", uts.machine);

    // 3. Получение системных метрик (не менее 3-х)
    long page_size = sysconf(_SC_PAGESIZE);
    long num_processors = sysconf(_SC_NPROCESSORS_ONLN);
    long num_pages = sysconf(_SC_PHYS_PAGES);
    printf("Размер страницы: %ld байт\n", page_size);
    printf("Количество процессоров: %ld\n", num_processors);
    printf("Количество страниц физической памяти: %ld\n", num_pages);

    // 4. Функции для работы со временем (не менее 2-х)
    time_t current_time;
    time(&current_time);
    printf("Текущее время: %s", ctime(&current_time));

    struct tm *local_time = localtime(&current_time);
    printf("Местное время: %02d:%02d:%02d\n",
           local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    // 5. Дополнительные API-функции (4 функции по выбору)
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("Текущий рабочий каталог: %s\n", cwd);

    pid_t pid = getpid();
    printf("PID процесса: %d\n", pid);

    uid_t uid = getuid();
    printf("UID пользователя: %d\n", uid);

    printf("Содержимое текущего каталога:\n");
    system("ls -l");

    return 0;
}
