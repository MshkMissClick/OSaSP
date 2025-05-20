#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Размер буфера для чтения файлов
#define BUFFER_SIZE 8192
#define HASH_SIZE 32

// Структура для хранения связей хэша и пути файла
typedef struct FileNode {
    char hash[HASH_SIZE + 1]; // Простая строка для хранения хэша
    char path[1024];
    struct FileNode* next;
} FileNode;

FileNode* hashTable = NULL;

// Функция вычисления хэша файла (имитация хэша через сумму байтов)
void calculateHash(const char* filepath, char* hashOutput) {
    // Инициализация более сложной хеш-функции
    unsigned long hash1 = 5381;  // Начальное значение для djb2
    unsigned long hash2 = 0;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    int fd;

    // Открываем файл
    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        strcpy(hashOutput, "error");
        return;
    }

    // Чтение файла и вычисление хеша
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytesRead; i++) {
            // Комбинируем два разных алгоритма хеширования
            hash1 = ((hash1 << 5) + hash1) + buffer[i];  // djb2
            hash2 = buffer[i] + (hash2 << 6) + (hash2 << 16) - hash2;  // sdbm
        }
    }

    // Проверка ошибок чтения
    if (bytesRead < 0) {
        perror("Error reading file");
        close(fd);
        strcpy(hashOutput, "error");
        return;
    }

    close(fd);

    // Комбинируем оба хеша для большей надежности
    unsigned long combined_hash = hash1 ^ hash2;

    // Преобразуем в hex-строку
    snprintf(hashOutput, HASH_SIZE + 1, "%016lx%016lx", hash1, hash2);
}

// Функция создания жёсткой ссылки
int createHardLink(const char* target, const char* duplicate) {
    return link(target, duplicate) == 0;
}

// Функция добавления файла в таблицу хэшей
void addToHashTable(const char* hash, const char* path) {
    FileNode* newNode = (FileNode*)malloc(sizeof(FileNode));
    strcpy(newNode->hash, hash);
    strcpy(newNode->path, path);
    newNode->next = hashTable;
    hashTable = newNode;
}

// Функция поиска дубликатов
const char* findDuplicate(const char* hash) {
    FileNode* current = hashTable;
    while (current) {
        if (strcmp(current->hash, hash) == 0)
            return current->path;
        current = current->next;
    }
    return NULL;
}

// Функция обхода директорий
void processDirectory(const char* dirPath, FILE* logFile) {
    struct dirent* entry;
    DIR* dir = opendir(dirPath);
    if (!dir) {
        perror("Error with opening directory");
        return;
    }

    struct stat fileStat;
    char filePath[1024];
    char hash[HASH_SIZE + 1];

    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);
        if (stat(filePath, &fileStat) == -1) {
            perror("Ошибка получения информации о файле");
            continue;
        }

        if (S_ISDIR(fileStat.st_mode)) {
            processDirectory(filePath, logFile);
        } else if (S_ISREG(fileStat.st_mode)) {
            calculateHash(filePath, hash);
            if (strlen(hash) == 0) continue;

            const char* duplicatePath = findDuplicate(hash);
            if (duplicatePath) {
                remove(filePath);
                if (createHardLink(duplicatePath, filePath)) {
                    printf("Duplicate: %s -> %s\n", filePath, duplicatePath);
                    fprintf(logFile, "Duplicate: %s -> %s\n", filePath, duplicatePath);
                } else {
                    perror("Ошибка создания жёсткой ссылки");
                }
            } else {
                addToHashTable(hash, filePath);
            }
        }
    }
    closedir(dir);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <путь_к_директории>\n", argv[0]);
        return 1;
    }

    FILE* logFile = fopen("duplicate_log.txt", "w");
    if (!logFile) {
        perror("Ошибка открытия лог-файла");
        return 1;
    }

    processDirectory(argv[1], logFile);

    fclose(logFile);


    return 0;
}
