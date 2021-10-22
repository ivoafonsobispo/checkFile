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
void extensionValidation(char *file_to_validate, Results *file_results); /* Function: Checks file extension validation */
char *returnFileExtension(char *filename, char c);                       /* Function: Returns the string of the extension */
