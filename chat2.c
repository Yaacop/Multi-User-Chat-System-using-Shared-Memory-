#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
int i;
#define FILLED 0
#define Ready 1
#define NotReady -1
#define MAX_HISTORY 10

struct memory {
    char buff[100];
    char sender[20]; // Username of sender
    int status, pid1, pid2;
    char history[MAX_HISTORY][100];
    int history_count;
};

struct memory* shmptr;

// Handler function to print message received from user1
void handler(int signum, siginfo_t *info, void *context)
{
    // If signum is SIGUSR1, then user 2 is receiving a message from user1
    if (signum == SIGUSR1) {
        printf("Received From User1 (%s): ", shmptr->sender);
        puts(shmptr->buff);
        // Check if user1 has exited
        if (strcmp(shmptr->buff, "User1 has left the chat system.\n") == 0) {
            printf("User1 has exited the chat system.\n");
            // Detach shared memory and exit
            shmdt((void*)shmptr);
            exit(0);
        }
        // Add the received message to history
        if (shmptr->history_count < MAX_HISTORY) {
            snprintf(shmptr->history[shmptr->history_count], sizeof(shmptr->history[0]), "%s: %s", shmptr->sender, shmptr->buff);
            shmptr->history_count++;
        } else {
            // If history is full, shift all messages and add the new message at the end
            for (i = 0; i < MAX_HISTORY - 1; i++) {
                strcpy(shmptr->history[i], shmptr->history[i + 1]);
            }
            snprintf(shmptr->history[MAX_HISTORY - 1], sizeof(shmptr->history[0]), "%s: %s", shmptr->sender, shmptr->buff);
        }
        // Append the message to the message history file
        FILE *history_file = fopen("message_history.txt", "a");
        if (history_file) {
            fprintf(history_file, "%s: %s\n", shmptr->sender, shmptr->buff);
            fclose(history_file);
        }
    }
}
int main()
{
    // Process id of user2
    int pid = getpid();

    int shmid;

    // Key value of shared memory
    int key = 1315;

    // Shared memory create
    shmid = shmget(key, sizeof(struct memory), IPC_CREAT | 0666);

    // Attaching the shared memory
    shmptr = (struct memory*)shmat(shmid, NULL, 0);

    // Store the process id of user2 in shared memory
    shmptr->pid2 = pid;

    shmptr->status = NotReady;
    shmptr->history_count = 0;

    // User authentication
    char username[20], password[20];
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    printf("Enter your password: ");
    fgets(password, sizeof(password), stdin);

    // Remove newline characters from username and password
    username[strcspn(username, "\n")] = '\0';
    password[strcspn(password, "\n")] = '\0';

    // Check if the entered username and password are correct
    // For simplicity, hardcoding a single user for now
    if (strcmp(username, "user2") != 0 || strcmp(password, "password2") != 0) {
        printf("Invalid username or password.\n");
        return 1;
    }

    // Save username in shared memory
    strcpy(shmptr->sender, username);

    // Load old messages from file and display them
    FILE* history_file = fopen("message_history.txt", "r");
    if (history_file) {
        char line[100];
        printf("Old Messages:\n");
        while (fgets(line, sizeof(line), history_file)) {
            printf("%s", line);
        }
        fclose(history_file);
    }

    // Set up signal handling using sigaction
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = &handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);

    while (1) {
        sleep(1);

        // Taking input from user2
        printf("User2 (%s): ", username);
        fgets(shmptr->buff, 100, stdin);
        shmptr->status = Ready;

        // Sending the message to user1 using kill function
        kill(shmptr->pid1, SIGUSR2);

        while (shmptr->status == Ready)
            continue;
    }

    shmdt((void*)shmptr);
    return 0;
}

