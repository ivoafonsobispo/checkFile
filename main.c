/**
 * @file    main.c
 * @brief   Main file for program usage
 * @date    2021-10-10
 * @author  Ivo Afonso Bispo 2200672
 * @author  Mariana Pereira 2200679
 */

/* Public libraries */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

/* Private libraries */
#include "args.h"
#include "debug.h"
#include "memory.h"

/* Defined variables */
#define NUM_VALID_EXTENSIONS 7
#define MAX_EXTENSION_SIZE 5

/* Structs */
typedef struct /* Struct to save the values for the summary output */
{
    int files_ok;
    int files_mismatch;
    int files_error;
    int files_analized;

} Results;

/* Created functions */
void outputFile(void);
void deleteFile(char *filename);
void extensionValidation(char *file_to_validate, Results *files);
char *returnFileExtension(char *filename, char c);

/* Global variables */
char output_file[16] = "temp-output.txt";

int main(int argc, char *argv[]) /* function: Main program execution */
{
    /* Create struct for gengtopt args */
    struct gengetopt_args_info args_info;

    /* Variables */
    int status;

    /* Verify if gengtopt args is valid */
    if (cmdline_parser(argc, argv, &args_info) != 0)
        ERROR(1, "cmdline_parser() failed!");

    /* fich */
    if (args_info.file_given)
    {
        Results files = {0, 0, 0, 0}; /* initialized struct */
        for (size_t i = 0; i < args_info.file_given; ++i)
        {
            switch (fork()) /* -1: error; 0: son process; default: parent process */
            {
            case -1: /* Code only executed in case of error */
                ERROR(1, "fork() failed!");
                break;

            case 0: /* Code only executed by the son process */
                /* Creates output file */
                outputFile();
                /* The calling process image is replaced by the image of the executable called via “exec” */
                execlp("file", "file", "-b", "--mime-type", args_info.file_arg[i], NULL);
                break;

            default: /* Code only executed by the parent process */
                waitpid(-1, &status, 0);
                /* Makes the file validations */
                extensionValidation(args_info.file_arg[i], &files);
                break;
            }
        }
    }

    /* fich_with_filenames */
    if (args_info.batch_arg)
    {
        /* Variables */
        char *batch_file_line = MALLOC(sizeof(char) + 1);
        size_t batch_file_len = 0;
        ssize_t batch_file_nread;
        Results files = {0, 0, 0, 0}; /* initialized struct */

        /* Verifies if file is txt */
        char *batch_file_extension = returnFileExtension(args_info.batch_arg, '.');
        if (strcmp(batch_file_extension, "txt") != 0)
            ERROR(1, "File: %s is not a txt", args_info.batch_arg);

        /* Opens file sent by user */
        FILE *fich_with_filenames = fopen(args_info.batch_arg, "r");
        if (fich_with_filenames == NULL)
            ERROR(1, "Failed to open the file '%s'", args_info.batch_arg);

        printf("[INFO] analyzing files listed in ‘%s’\n", args_info.batch_arg);

        while ((batch_file_nread = getline(&batch_file_line, &batch_file_len, fich_with_filenames)) != -1) /* Reads line from file */
        {
            switch (fork())
            {
            case -1: /* Code only executed in case of error */
                ERROR(1, "fork() failed!");
                break;

            case 0: /* Code only executed by the son process */
                /* Creates output file */
                outputFile();
                batch_file_line[strcspn(batch_file_line, "\n")] = 0;
                execlp("file", "file", "-b", "--mime-type", batch_file_line, NULL);
                break;

            default: /* Code only executed by the parent process */
                waitpid(-1, &status, 0);
                extensionValidation(batch_file_line, &files);
                break;
            }
        }

        printf("[SUMMARY] files analyzed : %d; files OK : %d; files MISMATCH : %d; errors: %d;\n", files.files_analized, files.files_ok, files.files_mismatch, files.files_error);

        fclose(fich_with_filenames);
        free(batch_file_line);
    }

    /* directory */
    if (args_info.dir_arg)
    {
        /* Variables */
        struct dirent *dir;
        Results files = {0, 0, 0, 0}; /* initialized struct */

        /* Opens path to directory */
        DIR *pDir = opendir(args_info.dir_arg);
        if (pDir == NULL)
            ERROR(1, "Could not open %s for reading", args_info.dir_arg);

        printf("[INFO] analyzing files of directory ‘%s’\n", args_info.dir_arg);

        while ((dir = readdir(pDir)) != NULL)
        {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;

            switch (fork())
            {
            case -1: /* Code only executed in case of error */
                ERROR(1, "fork() failed!");
                break;

            case 0: /* Code only executed by the son process */
                /* Creates output file */
                outputFile();
                execlp("file", "file", "-b", "--mime-type", dir->d_name, NULL);
                break;

            default: /* Code only executed by the parent process */
                waitpid(-1, &status, 0);
                extensionValidation(dir->d_name, &files);
                break;
            }
        }
        printf("[SUMMARY] files analyzed : %d; files OK : %d; files MISMATCH : %d; errors: %d;\n", files.files_analized, files.files_ok, files.files_mismatch, files.files_error);
    }

    /* Free of gengtopt args */
    cmdline_parser_free(&args_info);

    deleteFile(output_file);

    return 0;
}

void outputFile(void) /* Funtion: Creates the output file with user file extensions */
{
    /* Creates the output file */
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    /* Validates file creation */
    if (fd < 0)
        ERROR(1, "Can't open file - %s", output_file);

    /* Duplicates the descriptor “newfd” to “oldfd” */
    dup2(fd, STDOUT_FILENO);

    /* Validates closing file */
    if (close(fd) < 0)
        ERROR(1, "Closing - %s", output_file);
}

void extensionValidation(char *file_to_validate, Results *file_results) /* Function: Checks file extension validation */
{
    /* Variables */
    char valid_extension[NUM_VALID_EXTENSIONS][MAX_EXTENSION_SIZE] = {"pdf", "gif", "jpg", "png", "mp4", "zip", "html"};
    char *extension = MALLOC(sizeof(char) + 1);
    char *file_extension_out = NULL;  /* Extension from output file */
    char *file_extension_user = NULL; /* Extension from user file */
    size_t len = 0;
    ssize_t nread;
    int file_not_supported = 0;

    /* Opens output file + validates it */
    FILE *f = fopen(output_file, "r");
    if (f == NULL)
        ERROR(1, "Could not open temp-ouput.txt for reading");

    while ((nread = getline(&extension, &len, f)) != -1) /* Reads line from file */
    {
        /* Save the extension values */
        file_extension_out = returnFileExtension(extension, '/');
        file_extension_user = returnFileExtension(file_to_validate, '.');

        /* Deletes '\n' from file extensions */
        file_extension_out[strcspn(file_extension_out, "\n")] = 0;
        file_extension_user[strcspn(file_extension_user, "\n")] = 0;

        file_results->files_analized++;

        /* Searches in the file() output for the following string  */
        if (strstr(file_extension_out, "(No such file or directory)"))
        {
            printf("[ERROR] cannot open file ‘%s’ – No such file or directory\n", file_to_validate);
            file_results->files_error++;
            break;
        }

        file_not_supported = 0;
        for (int i = 0; i < NUM_VALID_EXTENSIONS; ++i)
        {
            /* Verifies if extension is a valid extension */
            if ((strcmp(file_extension_user, valid_extension[i])) == 0)
            {
                /* Verifies if the file user extension equals the output extension */
                if (strcmp(file_extension_out, file_extension_user) == 0 || strcmp(file_extension_out, "jpeg") == 0)
                {
                    printf("[OK] '%s': extension '%s' matches file type '%s'\n", file_to_validate, file_extension_user, valid_extension[i]);
                    file_results->files_ok++;
                    break;
                }
                printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", file_to_validate, file_extension_user, file_extension_out);
                file_results->files_mismatch++;
                break;
            }
            file_not_supported++;
        }
        /* Validates if the file sent is valid by checkfile */
        if (NUM_VALID_EXTENSIONS == file_not_supported)
        {
            printf("[INFO] '%s': type '%s' is not supported by checkFile\n", file_to_validate, file_extension_out);
            file_results->files_error++;
        }
    }

    fclose(f);
    free(extension);
}

char *returnFileExtension(char *filename, char c) /* Function: Returns the string of the extension */
{
    char *extension;
    /* Removes the character from the user file */
    extension = strrchr(filename, c);
    /* Validates if file extension exists */
    if (!extension || extension == filename)
        return filename;

    /* Changes the string to be the position after the removable character */
    return ++extension;
}

void deleteFile(char *filename) /* Function: Deletes file */
{
    switch (fork())
    {
    case -1: /* Code only executed in case of error */
        ERROR(1, "fork() failed!");
        break;

    case 0: /* Code only executed by the son process */
        execlp("rm", "rm", filename, NULL);
        break;

    default: /* Code only executed by the parent process */
        wait(NULL);
        break;
    }
}