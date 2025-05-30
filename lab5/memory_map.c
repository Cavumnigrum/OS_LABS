#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid> <output_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *pid = argv[1];
    char *output_dir = argv[2];

    // Открываем файл /proc/<pid>/maps
    char proc_maps_path[256];
    snprintf(proc_maps_path, sizeof(proc_maps_path), "/proc/%s/maps", pid);

    FILE *proc_file = fopen(proc_maps_path, "r");
    if (proc_file == NULL) {
        perror("Failed to open /proc/<pid>/maps");
        return EXIT_FAILURE;
    }

    // Формируем имя выходного файла
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char filename[512];
    snprintf(filename, sizeof(filename), "%s/map_%s_%04d-%02d-%02d_%02d-%02d-%02d.txt",
             output_dir, pid,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    FILE *out_file = fopen(filename, "w");
    if (out_file == NULL) {
        perror("Failed to create output file");
        fclose(proc_file);
        return EXIT_FAILURE;
    }

    // Копируем содержимое maps в выходной файл
    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), proc_file)) > 0) {
        fwrite(buffer, 1, bytes, out_file);
    }

    fclose(proc_file);
    fclose(out_file);

    printf("Memory map saved to: %s\n", filename);

    return EXIT_SUCCESS;
}
