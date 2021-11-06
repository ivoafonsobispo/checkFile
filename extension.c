/* File library */
#include "extension.h"

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

void split_path_file(char **p, char **f, char *pf) /* Function: To get file path and name */
{
    char *slash = pf, *next;
    while ((next = strpbrk(slash + 1, "\\/")))
        slash = next;
    if (pf != slash)
        slash++;
    *p = strndup(pf, slash - pf);
    *f = strdup(slash);
}