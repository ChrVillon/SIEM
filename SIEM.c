#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>

#define BUFFER_SIZE 2048
#define MAX_SERVICES 10
#define MAX_LOGS 1000

const char *all_priorities[] = {"emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"};

char* logs[MAX_SERVICES][MAX_LOGS];
int log_counts[MAX_SERVICES] = {0};

bool pflag = false;
char* priority = "";

void clear_screen() {
    printf("\033[H\033[J");
}

void print_help(char *command) {
    printf("Uso:\n %s <nombre_servicio_1.service> <nombre_servicio_2.service> ... [-p <nilvel de prioridad>] <Tiempo_de_actualizacion_en_segundos>\n", command);
    printf("Opciones\n");
    printf("-p\t\tMuestra los mensajes con con el nivel pripridad indicado (emerg, alert, crit, err, notice, info, debug).\n");
    printf("Aviso: Si no se especifica el tipo de prioridad solo se mostrara los mensajes de los ultimos 15 min.\n");
}

int main(int argc, char **argv)
{
    int opt; 

    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            pflag = true;
            priority = optarg;
            break;
        case 'h':
            print_help(argv[0]);
            return 0;
        default:
            printf("Uso:\n %s <nombre_servicio_1.service> <nombre_servicio_2.service> ... [-p <nilvel de prioridad>] <Actualizacion_en_segundos>\n", argv[0]);
            printf(" %s -h\n", argv[0]);
            return 1;
        }
    }
    

    int time = atoi(argv[argc - 1]);
    int num_services = argc - optind - 1;   
    int interval = atoi(argv[argc - 1]);
    pid_t pid;

    if (argc < 4)
    {        fprintf(stderr, "Uso: %s servicio1 [servicio2 ...] intervalo_segundos\n", argv[0]);
        exit(1);
        print_help(argv[0]);
    }

    clear_screen();
    printf("╔═══════════════════════╦════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ Servicio              ║ Conteo por prioridad                                                               ║\n");
    printf("╠═══════════════════════╬════════════════════════════════════════════════════════════════════════════════════╣\n");

    for (int i = 0; i < num_services; i++)
    {
        int pipefd[2];
        pipe(pipefd);

        pid = fork();
        if (pid == 0)
        {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            execlp("journalctl", "journalctl", "-u", argv[i + 1], "--output=json", NULL);
            perror("execlp");
            exit(1);
        }

        close(pipefd[1]);

        FILE *fp = fdopen(pipefd[0], "r");

        if (!fp) {
            perror("fdopen");
            continue;
        }

        int priority_number[8] = {0};
        char line[BUFFER_SIZE];

        while (fgets(line, sizeof(line), fp))
        {
            for (int p = 0; p < sizeof(priority_number); p++)
            {
                char key[24];
                snprintf(key, sizeof(key),"\"PRIORITY\":\"%d\"", p);
                if (strstr(line, key))
                {
                    priority_number[p]++;
                    break;
                }
            }
            if (log_counts[i] < MAX_LOGS)
                logs[i][log_counts[i]++] = strdup(line);  
        }
        fclose(fp);
        wait(NULL);

        printf("║ %-21s ║", argv[i + 1]);
        for (int p = 0; p < 8; p++) {
            printf(" %s:%3d", all_priorities[p], priority_number[p]);
            if (p < 7) printf(" |");
        }
        printf(" ║\n");
    }

    printf("╚═══════════════════════╩════════════════════════════════════════════════════════════════════════════════════╝\n");
    printf("Actualizado cada %d segundos. Ctrl+C para salir.\n", interval);

    for (int i = 0; i < num_services; i++)
    {
        printf("Mensajes de %s\n", argv[1 + i]);
        for (int l = 0; l < log_counts[i]; l++)
        {
            char* start = strstr(logs[i][l], "\"MESSAGE\":\"");
            if (start)
            {
                start += strlen("\"MESSAGE\":\"");
                char* end = strchr(start, '\"');
                if (end)
                {
                    *end = '\0';
                    printf("-%s\n", start);
                }
                
            }
        }
        
    }
    

    sleep(interval);

    return 0;
}