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
#include <signal.h>
#include <time.h>

/* Private libraries */
#include "args.h"
#include "debug.h"
#include "memory.h"
#include "extension.h"

/* Created functions */
void outputFile(void);
void deleteFile(char *filename);
void treatSignalInfo(int signal, siginfo_t *siginfo, void *context);

/* Global Variables */
/* Flag to wait for the right signal */
int sig_SIGQUIT = 1;
int sig_SIGINT = 1;
int sig_SIGUSR1 = 1;
char *batch_filename = NULL;

int main(int argc, char *argv[]) /* function: Main program execution */
{
    /* Create struct for gengtopt args */
    struct gengetopt_args_info args_info;

    /* Create struct for singal treatment */
    struct sigaction act_info;

    /* Variables */
    int status;

    /* Signals */
    act_info.sa_sigaction = treatSignalInfo;
    sigemptyset(&act_info.sa_mask);
    act_info.sa_flags = 0;
    act_info.sa_flags |= SA_SIGINFO; /* Adicional info about signals */

    /* Verify if gengtopt args is valid */
    if (cmdline_parser(argc, argv, &args_info) != 0)
        ERROR(1, "cmdline_parser() failed!");

    /* Verifications for signals */
    if (sigaction(SIGQUIT, &act_info, NULL) < 0)
        ERROR(1, "sigaction(SIGQUIT) failed!");
    if (sigaction(SIGINT, &act_info, NULL) < 0)
        ERROR(1, "sigaction(SIGINT) failed!");
    if (sigaction(SIGUSR1, &act_info, NULL) < 0)
        ERROR(1, "sigaction(SIGUSR1) failed!");

    /* Ignore SIGUSR1 signal if not batch/fich_with_filenames */
    if (!args_info.batch_arg)
        signal(SIGUSR1, SIG_IGN);

    /* fich */
    if (args_info.file_given)
    {
        /* Asks for signal and processeds with application */
        printf("Please send a SIGQUIT to the process PID: %d\nUsage: kill -s SIGQUIT <PID>\n\n", getpid());

        /* Waits for the right signal */
        while (sig_SIGQUIT)
            pause();

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
        /* Waits for the right signal */
        while (sig_SIGINT)
            pause();
    }

    /* fich_with_filenames */
    if (args_info.batch_arg)
    {

        /* Asks for signal and processeds with application */
        printf("Please send a SIGQUIT or SIGUSR1 to the process PID: %d\nUsage: kill -s SIGQUIT <PID>\n\n", getpid());

        batch_filename = args_info.batch_arg;

        /* Waits for the right signal */
        while (sig_SIGQUIT && sig_SIGUSR1)
            pause();

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

        /* Waits for the right signal */
        while (sig_SIGINT)
            pause();
    }

    /* directory */
    if (args_info.dir_arg)
    {
        /* Asks for signal and processeds with application */
        printf("Please send a SIGQUIT to the process PID: %d\nUsage: kill -s SIGQUIT <PID>\n\n", getpid());

        /* Waits for the right signal */
        while (sig_SIGQUIT)
            pause();

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
                /* Adds "/" to know if file is directory */
                strcat(args_info.dir_arg, "/");
                strcat(args_info.dir_arg, dir->d_name);
                /* Creates output file */
                outputFile();
                execlp("file", "file", "-b", "--mime-type", args_info.dir_arg, NULL);
                break;

            default: /* Code only executed by the parent process */
                waitpid(-1, &status, 0);
                extensionValidation(dir->d_name, &files);
                break;
            }
        }
        printf("[SUMMARY] files analyzed : %d; files OK : %d; files MISMATCH : %d; errors: %d;\n", files.files_analized, files.files_ok, files.files_mismatch, files.files_error);

        /* Waits for the right signal */
        while (sig_SIGINT)
            pause();
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

void treatSignalInfo(int signal, siginfo_t *siginfo, void *context)
{
    (void)context;
    int aux = errno;

    /* Gets current time */
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    switch (signal)
    {
    case 3: /* SIGQUIT value = 3 */
        printf("Captured SIGQUIT signal (sent by PID: %ld). Use SIGINT to terminate application.\n\n", (long)siginfo->si_pid);
        sig_SIGQUIT = 0; /* Stop the loop */
        break;

    case 2: /* SIGINT value = 2 */
        printf("Captured SIGINT signal (sent by PID: %ld).\n\n", (long)siginfo->si_pid);
        sig_SIGINT = 0; /* Stop the loop */
        break;

    default: /* SIGUSR1 value = diferent values */
        printf("Captured SIGUSR1 signal (sent by PID: %ld). Use SIGINT to terminate application.\n", (long)siginfo->si_pid);
        printf("Start Process at %d.%02d.%02d_%02dh%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        printf("Processing file nº%d/%s\n\n", getpid(), batch_filename);
        sig_SIGUSR1 = 0; /* Stop the loop */
        break;
    }

    errno = aux;
}