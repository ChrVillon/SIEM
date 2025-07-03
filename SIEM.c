#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048

const char *all_priorities[] = {"emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"};

int priority_level(char* name)
{
    if (strcmp(name, "emerg") == 0) return 0;
    if (strcmp(name, "alert") == 0) return 1;
    if (strcmp(name, "crit") == 0) return 2;
    if (strcmp(name, "err") == 0) return 3;
    if (strcmp(name, "warning") == 0) return 4;
    if (strcmp(name, "notice") == 0) return 5;
    if (strcmp(name, "info") == 0) return 6;
    if (strcmp(name, "debug") == 0) return 7;
    return -1;
}

void nameset_input_nonblocking()
{
    int flags = fcntl(STDIN_FILENO, F_GETFD, 0);
    fcntl(STDIN_FILENO, F_SETFD, flags | O_NONBLOCK);
}

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

    nameset_input_nonblocking();

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

    for (int i = 0; i < num_services; i++) {
        FILE *fp = fdopen(pipes[i][0], "r");
        if (!fp) {
            perror("fdopen");
            continue;
        }
    
        int emerg = 0, alert = 0, crit = 0, err = 0;
        int warning = 0, notice = 0, info = 0, debug = 0;
    
        char line[BUFFER_SIZE];
        char priority_input[16] = "";
        int priority = -1;
    
        char input[32];
        if (fgets(input, sizeof(input), stdin)) {
            input[strcspn(input, "\n")] = 0;
            strcpy(priority_input, input);
            priority = priority_level(priority_input);
        }
    
        while (fgets(line, sizeof(line), fp))
        {
            if (strstr(line, "\"PRIORITY\":\"0\"")) emerg++;
            else if (strstr(line, "\"PRIORITY\":\"1\"")) alert++;
            else if (strstr(line, "\"PRIORITY\":\"2\"")) crit++;
            else if (strstr(line, "\"PRIORITY\":\"3\"")) err++;
            else if (strstr(line, "\"PRIORITY\":\"4\"")) warning++;
            else if (strstr(line, "\"PRIORITY\":\"5\"")) notice++;
            else if (strstr(line, "\"PRIORITY\":\"6\"")) info++;
            else if (strstr(line, "\"PRIORITY\":\"7\"")) debug++;
    
            for (int p = 0; p <= 7; p++)
            {
                char key[20];
                snprintf(key, sizeof(key), "\"PRIORITY\":\"%d\"", p);
                if (strstr(line, key)) {
                    if (priority == p) {
                        printf("%s", line);
                    }
                    break;
                }
            }
        }
    
        fclose(fp);
    
        printf("\nServicio: %s\n", argv[i + 1]);
        printf("  emerg  : %d\n", emerg);
        printf("  alert  : %d\n", alert);
        printf("  crit   : %d\n", crit);
        printf("  err    : %d\n", err);
        printf("  warning: %d\n", warning);
        printf("  notice : %d\n", notice);
        printf("  info   : %d\n", info);
        printf("  debug  : %d\n", debug);
        printf("--------------------------------------\n");
    }
    
    for (int i = 0; i < num_services; i++)
    {
        wait(NULL);
    }

    return 0;
}