#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>


#define ERROR -1
#define SUCCESS 0


enum CONSTANTS {
    READ_AND_WRITE_PERMISSIONS = 0777,
    MAX_PATH_FILE = 4096,
    POSITION_FILE_PATH = 1,
    BLOCK_SIZE = 20,
    ZERO_OFFSET_RELATIVE_TO_SEEK_SET = 0,
    ZERO = 0,
    MAX_PATH_DIRECTORY = 4096,
    FILE_NAME_MAX = 256
};

void reverse_string(char *str) {
    size_t length = strlen(str);
    size_t half_length = length / 2;
    for (int i = ZERO; i < half_length; i++) {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}

void reverse_base_filename(char *file_name) {
    char *dot = strrchr(file_name, '.');
    if (dot == NULL) {
        reverse_string(file_name);
        return;
    }
    int position_to_dot = dot - file_name - 1;
    int i = ZERO;
    int j = position_to_dot;
    while (i < j) {
        char temp = file_name[i];
        file_name[i] = file_name[j];
        file_name[j] = temp;
        i++;
        j--;
    }
}

void print_error(const char *file_path) {
    fprintf(stderr, "%s : %s\n", file_path, strerror(errno));
}

int write_buffer_in_new_file(FILE *source_file, FILE *new_file, int count_block, int number_block, int size_block) {
    char *buffer = (char *) calloc(size_block, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error in write_buffer_in_new_file, %s \n", strerror(errno));
        return ERROR;
    }
    int return_value_fseek;
    if(count_block - number_block != 0){
        return_value_fseek = fseek(source_file, -((number_block+1) * BLOCK_SIZE), SEEK_END);
    }
    else {
        return_value_fseek = fseek(source_file, 0, SEEK_SET);
    }
    if (return_value_fseek != 0) {
        fprintf(stderr, "Error in fseek from write_buffer_in_new_file, %s \n", strerror(errno));
        free(buffer);
        return ERROR;
    }
    int return_value_fread = fread(buffer, sizeof(char), size_block, source_file);
    if (return_value_fread != size_block) {
        fprintf(stderr, "Error in fread from write_buffer_in_new_file, %s \n", strerror(errno));
        free(buffer);
        return ERROR;
    }

    return_value_fseek = fseek(new_file, BLOCK_SIZE*number_block, SEEK_SET);
    if (return_value_fseek != 0) {
        fprintf(stderr, "Error in fseek from write_buffer_in_new_file, %s \n", strerror(errno));
        free(buffer);
        return ERROR;
    }
    for(int i=0;i<size_block;i++){
        fprintf(new_file, "%c", buffer[size_block-i-1]);
    }
    
    free(buffer);
    return SUCCESS;
}

int close_files(FILE* source_file, FILE* new_file){
    int return_value_fclose_source_file = fclose(source_file);
    int return_value_fclose_new_file = fclose(new_file);
    if (return_value_fclose_source_file == EOF || return_value_fclose_new_file == EOF) {
        fprintf(stderr, "Error in fclose from , %s \n", strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}


int copy_file(char *path_source_file, char *path_new_file) {
    FILE *source_file;
    FILE *new_file;
    source_file = fopen(path_source_file, "rb");
    if (source_file == NULL) {
        print_error(path_source_file);
        return ERROR;
    }
    new_file = fopen(path_new_file, "wb");
    if (new_file == NULL) {
        print_error(path_new_file);
        return ERROR;
    }

    int return_value_fseek = fseek(source_file, ZERO_OFFSET_RELATIVE_TO_SEEK_SET, SEEK_END);
    if (return_value_fseek != 0) {
        fprintf(stderr, "Error in fseek from copy_file, %s \n", strerror(errno));
        if(close_files(source_file, new_file) == ERROR){
            return ERROR;
        }
        return ERROR;
    }
    int file_size = ftell(source_file);
    if (file_size == -1L) {
        fprintf(stderr, "Error in ftell from copy_file, %s \n", strerror(errno));
        if(close_files(source_file, new_file) == ERROR){
            return ERROR;
        }
        return ERROR;
    }
    int count_blocks = file_size / BLOCK_SIZE;
    int remainder = file_size % BLOCK_SIZE;
    int number_block;
    int return_value_write_buffer_in_new_file;
    for (number_block = ZERO; number_block < count_blocks; number_block++) {
        return_value_write_buffer_in_new_file = write_buffer_in_new_file(source_file, new_file, count_blocks,
                                                                         number_block, BLOCK_SIZE);
        if (return_value_write_buffer_in_new_file == ERROR) {
            if(close_files(source_file, new_file) == ERROR){
                return ERROR;
            }
            return ERROR;
        }
    }
    if (remainder > ZERO) {
        return_value_write_buffer_in_new_file = write_buffer_in_new_file(source_file, new_file, count_blocks,
                                                                         number_block, remainder);
        if (return_value_write_buffer_in_new_file == ERROR) {
            if(close_files(source_file, new_file) == ERROR){
                return ERROR;
            }
            return ERROR;
        }
    }

    if (close_files(source_file, new_file) == ERROR){
        return ERROR;
    }

    return SUCCESS;
}

int get_name_directory(char *path_source_directory, char *name_new_directory) {
    char *ptr_last_slash = strrchr(path_source_directory, '/');
    if (ptr_last_slash == NULL) {
        fprintf(stderr, "%s don't have slash, error in name directory \n", path_source_directory);
        return ERROR;
    }
    size_t size_name_directory = strlen(ptr_last_slash) - 1;
    for (int i = ZERO; i < size_name_directory; i++) {
        name_new_directory[i] = ptr_last_slash[i + 1];
    }
    return SUCCESS;
}

int get_name_new_directory(char *path_source_directory, char *name_new_directory) {
    int return_value_get_name_directory = get_name_directory(path_source_directory, name_new_directory);
    if (return_value_get_name_directory == ERROR) {
        return ERROR;
    }
    reverse_string(name_new_directory);
    return SUCCESS;
}

int get_pos_last_slash(char *ptr_last_slash, char *path_source_directory) {
    int pos_last_slash = strlen(path_source_directory) - strlen(ptr_last_slash) + 1;
    return pos_last_slash;
}

int get_path_new_directory(char *path_source_directory, char *path_new_directory) {
    sprintf(path_new_directory, "%s", path_source_directory);
    char *name_new_directory;
    char *ptr_last_slash = strrchr(path_source_directory, '/');
    if (ptr_last_slash == NULL) {
        fprintf(stderr, "%s don't have slash, error in name directory \n", path_source_directory);
        return ERROR;
    }
    size_t size_name_directory = strlen(ptr_last_slash) - 1;
    name_new_directory = (char *) malloc(size_name_directory * sizeof(char));
    if (name_new_directory == NULL) {
        fprintf(stderr, "Memory allocation error in function get_path_new_directory, %s \n", strerror(errno));
        return ERROR;
    }
    int return_value_get_name_new_directory = get_name_new_directory(path_source_directory, name_new_directory);
    if (return_value_get_name_new_directory == ERROR) {
        free(name_new_directory);
        return ERROR;
    }
    int pos_last_slash = get_pos_last_slash(ptr_last_slash, path_source_directory);
    int length_path_source_directory = strlen(path_source_directory);
    int i = 0;
    while (pos_last_slash != length_path_source_directory) {
        path_new_directory[pos_last_slash] = name_new_directory[i];
        i++;
        pos_last_slash++;
    }
    free(name_new_directory);
    return SUCCESS;
}

int copy_directory(char *path_source_directory, char *path_new_directory) {
    DIR *directory;
    struct dirent *ent;
    directory = opendir(path_source_directory);
    if (directory == NULL) {
        print_error(path_source_directory);
        return ERROR;
    }

    ent = readdir(directory);
    int compare_with_one_dot = strcmp(ent->d_name, ".");
    int compare_with_two_dot = strcmp(ent->d_name, "..");
    mkdir(path_new_directory, READ_AND_WRITE_PERMISSIONS);

    while (ent != NULL) {
        if (ent->d_type == DT_DIR && (compare_with_one_dot != 0) && (compare_with_two_dot != 0)) {
            char current_path_source_directory[MAX_PATH_DIRECTORY];
            sprintf(current_path_source_directory, "%s/%s", path_source_directory, ent->d_name);
            char current_path_new_directory[MAX_PATH_DIRECTORY];
            char current_new_name_directory[FILE_NAME_MAX];
            sprintf(current_new_name_directory, "%s", ent->d_name);
            reverse_string(current_new_name_directory);
            sprintf(current_path_new_directory, "%s/%s", path_new_directory, current_new_name_directory);
            int return_value_copy_directory = copy_directory(current_path_source_directory, current_path_new_directory);
            if (return_value_copy_directory == ERROR) {
                print_error(current_path_source_directory);
                return ERROR;
            }
        }

        if (ent->d_type == DT_REG) {
            char path_source_file[MAX_PATH_FILE];
            char path_new_file[MAX_PATH_FILE];
            char reversed_name_file[FILE_NAME_MAX];
            sprintf(path_source_file, "%s/%s", path_source_directory, ent->d_name);
            sprintf(reversed_name_file, "%s", ent->d_name);
            reverse_base_filename(reversed_name_file);
            sprintf(path_new_file, "%s/%s", path_new_directory, reversed_name_file);
            int return_value_copy_file = copy_file(path_source_file, path_new_file);
            if (return_value_copy_file == ERROR) {
                return ERROR;
            }
            printf("Copied file: %s\n", ent->d_name);
        }
        ent = readdir(directory);
        if (ent != NULL) {
            compare_with_one_dot = strcmp(ent->d_name, ".");
            compare_with_two_dot = strcmp(ent->d_name, "..");
        }
    }
    closedir(directory);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    char *path_source_directory = argv[POSITION_FILE_PATH];
    size_t size_path_source_directory = strlen(path_source_directory);
    char *path_new_directory = malloc(size_path_source_directory * sizeof(char));
    if (path_new_directory == NULL) {
        fprintf(stderr, "Memory allocation error in function main, %s \n", strerror(errno));
        return EXIT_FAILURE;
    }
    int return_value_get_path_new_directory = get_path_new_directory(path_source_directory, path_new_directory);
    if (return_value_get_path_new_directory == ERROR) {
        free(path_new_directory);
        return EXIT_FAILURE;
    }
    int return_value_copy_directory = copy_directory(path_source_directory, path_new_directory);
    if (return_value_copy_directory == ERROR) {
        free(path_new_directory);
        return EXIT_FAILURE;
    }
    printf("Finished copying all files.\n");
    free(path_new_directory);
    return EXIT_SUCCESS;
}

