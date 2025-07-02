#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 2048

const char *all_priorities[] = {"emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"};

int main(int argc, char **argv)
{
    int time = atoi(argv[argc - 1]);
    int num_services = argc - 2;
    pid_t pid;
    int pipes[num_services][2];

    if (argc < 4)
    {
        fprintf(stderr, "Uso: %s servicio1 [servicio2 ...] intervalo_segundos\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < num_services; i++)
    {
        pipe(pipes[i]);
        pid = fork();

        if (pid == 0)
        {
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);

            execlp("journalctl", "journalctl", "-u", argv[i + 1], "--output=json", NULL);
            perror("execlp");
            exit(1);
        }

        close(pipes[i][1]);
    }

    char buffer[BUFFER_SIZE];
    for (int i = 0; i < num_services; i++)
    {
        int emerg = 0, alert = 0, crit = 0, err = 0, warning = 0, notice = 0, info = 0, debug = 0;

        ssize_t n;
        while ((n = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[n] = '\0';

            char *linea = strtok(buffer, "\n");
            while (linea != NULL)
            {
                if (strstr(linea, "\"PRIORITY\":\"0\""))
                {
                    emerg++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"1\""))
                {
                    alert++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"2\""))
                {
                    crit++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"3\""))
                {
                    err++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"4\""))
                {
                    warning++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"5\""))
                {
                    notice++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"6\""))
                {
                    info++;
                }
                else if (strstr(linea, "\"PRIORITY\":\"7\""))
                {
                    debug++;
                }

                linea = strtok(NULL, "\n");
            }
        }

        close(pipes[i][0]);

        printf("Servicio: %s\n", argv[i + 1]);
        printf("  emerg  : %d\n", emerg);
        printf("  alert  : %d\n", alert);
        printf("  crit   : %d\n", crit);
        printf("  err    : %d\n", err);
        printf("  warning: %d\n", warning);
        printf("  notice : %d\n", notice);
        printf("  info   : %d\n", info);
        printf("  debug  : %d\n", debug);
        printf("-------------------------------------\n");
    }

    for (int i = 0; i < num_services; i++) {
        wait(NULL);
    }

    return 0;
}