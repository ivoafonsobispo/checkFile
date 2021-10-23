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

/* Created functions */
void treatSignalInfo(int signal, siginfo_t *siginfo, void *context);
