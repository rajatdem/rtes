#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define SIGEXCESS 33

void format_error()
{
    printf("Command format error\n");
    printf("energymon <pid>\n");
    exit(0);
}

void sigexcess_handler(int sig)
{
    printf("Caught signal: SIGEXCESS\n");
}

int main(int argc, char *argv[])
{
     int energy_pid;
     char str1[100] = {0};
     char energy_path[100] = {0};
     int pid = atoi(argv[1]);
     sprintf(energy_path, "/sys/rtes/tasks/%d/energy", pid);
     FILE *freq, *power, *energy, *pid_energy;
     printf("ARGC: %d\n", argc);
     if(SIG_ERR == signal(SIGEXCESS, sigexcess_handler))
     {
        printf("Could not register a signal handler\n");
     }
     if(argc != 2)
     {
        format_error();
     }
     if(atoi(argv[1]) == 0)
     {
        printf("FREQ (MHZ)\tPOWER (mW)\tENERGY (mJ)\n");
        while(1)
        {
            freq = fopen("/sys/rtes/freq", "r"); 
            power = fopen("/sys/rtes/power", "r"); 
            energy = fopen("/sys/rtes/energy", "r"); 
            if(fscanf(freq, "%s", str1)!=0)
            {
                printf("%s\t\t", str1);
                fclose(freq);
                memset(str1, 0, strlen(str1));
            }
            if(fscanf(power, "%s", str1)!=0)
            {
                printf("%s\t\t", str1);
                fclose(power);
                memset(str1, 0, strlen(str1));
            }
            if(fscanf(energy, "%s", str1)!=0)
            {
                printf("%s\n", str1);
                fclose(energy);
                memset(str1, 0, strlen(str1));
            }        
            sleep(1);
        }
     }
     else
     {
         if((energy_pid = open(energy_path, O_RDONLY))>=0)
         {
             close(energy_pid);
             printf("FREQ (MHZ)\tPOWER (mW)\tENERGY (mJ)\n");
             while(1)
             {
                freq = fopen("/sys/rtes/freq", "r"); 
                power = fopen("/sys/rtes/power", "r"); 
                if(fscanf(freq, "%s", str1)!=0)
                {
                    printf("%s\t\t", str1);
                    fclose(freq);
                    memset(str1, 0, strlen(str1));
                }
                if(fscanf(power, "%s", str1)!=0)
                {
                    printf("%s\t\t", str1);
                    fclose(power);
                    memset(str1, 0, strlen(str1));
                }
                energy_pid = open(energy_path, O_RDONLY);
                read(energy_pid, str1, 15);
                printf("%s", str1);
                close(energy_pid);
                memset(str1, 0, strlen(str1));
                sleep(1);
             }
         }
         else
         {
            printf("Task: %d is not being monitored or doesn't exist\n", atoi(argv[1]));
         }
     }
    return 0;
}
