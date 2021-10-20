/**
 * @file main.c
 * @brief Main file for program usage
 * @date 2021-10-10
 * @author Ivo Afonso Bispo 2200672
 * @author Mariana Mariana 2200672
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

/* Private libraries */
#include "args.h"
#include "debug.h"
#include "memory.h"

/* Defined variables */
#define NUM_VALID_EXTENSIONS 7
#define MAX_EXTENSION_SIZE 5

/* Created functions */
void outputFile(void);
void deleteFile(char *filename);
void extensionValidation(char *file_to_validate);
char *returnFileExtension(char *filename, char c);

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
                extensionValidation(args_info.file_arg[i]);
                break;
            }
        }
    }

    /* Free of gengtopt args */
    cmdline_parser_free(&args_info);

    deleteFile("temp-output.txt");

    return 0;
}

void outputFile(void) /* Funtion: Creates the output file with user file extensions */
{
    /* Creates the output file */
    int fd = open("temp-output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    /* Validates file creation */
    if (fd < 0)
        ERROR(1, "Can't open file - temp-output.txt");

    /* Duplicates the descriptor “newfd” to “oldfd” */
    dup2(fd, STDOUT_FILENO);

    /* Validates closing file */
    if (close(fd) < 0)
        ERROR(1, "Closing - temp-output.txt");
}

void extensionValidation(char *file_to_validate) /* Function: Checks file extension validation */
{
    /* Variables */
    char valid_extension[NUM_VALID_EXTENSIONS][MAX_EXTENSION_SIZE] = {"pdf", "gif", "jpg", "png", "mp4", "zip", "html"};
    char *extension = MALLOC(sizeof(char) + 1);
    char *file_extension_out = NULL;  /* Extension from output file */
    char *file_extension_user = NULL; /* Extension from user file */
    size_t len = 0;
    ssize_t nread;
    int files_ok = 0, files_mismatch = 0, file_not_supported = 0, files_erros = 0, files_analyzed = 0;

    /* Opens output file + validates it */
    FILE *f = fopen("temp-output.txt", "r");
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

        file_not_supported = 0;
        for (int i = 0; i < NUM_VALID_EXTENSIONS; ++i)
        {
            /* Verifies if extension is valid */
            if ((strcmp(file_extension_user, valid_extension[i])) == 0)
            {
                /* Verifica se a extensão é a correta */
                if (strcmp(file_extension_user, file_extension_out) || (strcmp("jpeg", file_extension_out)))
                {
                    printf("[OK] '%s': extension '%s' matches file type '%s'\n", file_to_validate, file_extension_user, valid_extension[i]);
                    files_ok++;
                    break;
                }
                printf("[MISMATCH] '%s': extension is '%s', file type is '%s'\n", file_to_validate, file_extension_user, file_extension_out);
                files_mismatch++;
                break;
            }
            file_not_supported++;
        }
        if (NUM_VALID_EXTENSIONS == file_not_supported)
        {
            printf("[INFO] '%s': type '%s' is not supported by checkFile\n", file_extension_user, file_extension_out);
            files_erros++;
        }
        files_analyzed++;
    }

    printf("[SUMMARY] files analyzed : %d; files OK : %d; files MISMATCH : %d; errors: %d;\n", files_analyzed, files_ok, files_mismatch, files_erros);

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
        ERROR(1, "Error finding file extension");

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