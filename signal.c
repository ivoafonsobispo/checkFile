/* File library */
#include "signal.h"

void treatSignalInfo(int signal, siginfo_t *siginfo, void *context) /* Function: To tread and output the signal information */
{
    (void)context;

    /* Copies the global variable errno */
    int aux = errno;

    printf("Recebi o sinal (%d)\n", signal);
    printf("Detalhes:\n");
    printf("\tPID: %ld\n\tUID: %ld\n", (long)siginfo->si_pid, (long)siginfo->si_uid);

    /* Restaura valor da variável global errno */
    errno = aux;
}