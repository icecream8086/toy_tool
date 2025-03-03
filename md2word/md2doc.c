#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define realpath(N, R) _fullpath((R), (N), _MAX_PATH)
#else
#include <dirent.h>
#include <limits.h>
#endif

#define MAX_PATH_LEN 1024
#define MAX_CMD_LEN 4096

void convert_file(const char *input_path, const char *options);
void process_directory(const char *dir_path, const char *options);
void print_help();

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_path> [pandoc_options...]\n", argv[0]);
        fprintf(stderr, "Use -h for help.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return EXIT_SUCCESS;
    }

    char abs_input_path[MAX_PATH_LEN];
    if (realpath(argv[1], abs_input_path) == NULL) {
        perror("Error resolving input path");
        return EXIT_FAILURE;
    }

    struct stat path_stat;
    if (stat(abs_input_path, &path_stat) != 0) {
        perror("Error accessing input path");
        return EXIT_FAILURE;
    }

    char options[MAX_CMD_LEN] = "";
    for (int i = 2; i < argc; ++i) {
        strncat(options, argv[i], MAX_CMD_LEN - strlen(options) - 1);
        strncat(options, " ", MAX_CMD_LEN - strlen(options) - 1);
    }

    if (S_ISREG(path_stat.st_mode)) {
        convert_file(abs_input_path, options);
    } else if (S_ISDIR(path_stat.st_mode)) {
        process_directory(abs_input_path, options);
    } else {
        fprintf(stderr, "Input path is neither a file nor a directory\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void convert_file(const char *input_path, const char *options) {
    char output_path[MAX_PATH_LEN];
    strncpy(output_path, input_path, MAX_PATH_LEN - 1);
    output_path[MAX_PATH_LEN - 1] = '\0';

    char *last_dot = strrchr(output_path, '.');
    if (last_dot != NULL) *last_dot = '\0';
    strncat(output_path, ".docx", MAX_PATH_LEN - strlen(output_path) - 1);

    char dir_path[MAX_PATH_LEN];
    strncpy(dir_path, input_path, MAX_PATH_LEN - 1);
    dir_path[MAX_PATH_LEN - 1] = '\0';
    char *last_slash = strrchr(dir_path, '/');
#ifdef _WIN32
    if (!last_slash) last_slash = strrchr(dir_path, '\\');
#endif
    if (last_slash) *last_slash = '\0';

    char command[MAX_CMD_LEN];
    snprintf(command, MAX_CMD_LEN, "pandoc \"%s\" %s -o \"%s\" --resource-path=\"%s\"", 
            input_path, options, output_path, dir_path);

    printf("Converting: %s\n", input_path);
    int ret = system(command);
    
    if (ret != 0) {
        fprintf(stderr, "Conversion failed for %s (Error code: %d)\n", 
                input_path, ret);
    } else {
        printf("Success: %s -> %s\n", input_path, output_path);
    }
}

void process_directory(const char *dir_path, const char *options) {
#ifdef _WIN32
    char search_path[MAX_PATH_LEN];
    snprintf(search_path, MAX_PATH_LEN, "%s\\*.md", dir_path);

    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char file_path[MAX_PATH_LEN];
            snprintf(file_path, MAX_PATH_LEN, "%s\\%s", dir_path, find_data.cFileName);
            convert_file(file_path, options);
        }
    } while (FindNextFile(hFind, &find_data));

    FindClose(hFind);
#else
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Directory open error");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG) {
            const char *name = entry->d_name;
            size_t len = strlen(name);
            if (len > 3 && strcmp(name + len - 3, ".md") == 0) {
                char file_path[MAX_PATH_LEN];
                snprintf(file_path, MAX_PATH_LEN, "%s/%s", dir_path, name);
                convert_file(file_path, options);
            }
        }
    }
    closedir(dir);
#endif
}

void print_help() {
    printf("Markdown to Word Converter\n");
    printf("Usage: md2word <input_path> [pandoc_options...]\n\n");
    printf("Arguments:\n");
    printf("  <input_path>    Absolute or relative path to a Markdown file/directory\n");
    printf("  [pandoc_options] Optional Pandoc options (e.g., --toc, -s, --reference-doc)\n\n");
    printf("Examples:\n");
    printf("  Convert a file: md2word ~/doc.md --toc\n");
    printf("  Convert a directory: md2word ./notes --reference-doc template.docx\n");
    printf("\nNote: Always uses absolute paths internally for reliability\n");
}