#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <pthread.h>

#define SHORT_SLEEP 1
#define LONG_SLEEP 3

size_t allocBytes = 1024 * 1024 * 100;

void print_memory_usage(char *id) {
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    printf("%s:\t using memory %ld\n", id, r_usage.ru_maxrss);
}

void allocate_memory(char *id, size_t length) {
    char *allocatedMemory;
    printf("%s\t allocating memory\n", id);
    allocatedMemory = malloc(length);
    memset(allocatedMemory, 42, length);
}

void sleep_wrapper(char *id, int duration) {
    printf("%s:\t sleeping for %d seconds\n", id, duration);
    sleep(duration);
}

void* thread_entry(void *arg) {
    printf("Thread:\t entry\n");
    sleep_wrapper("Thread", SHORT_SLEEP);
    allocate_memory("Thread", allocBytes);
    print_memory_usage("Thread");
    sleep_wrapper("Thread", LONG_SLEEP);
    printf("Thread:\t exit\n");
    return NULL;
}

int main(int argc, char **argv) {
    pid_t processId;
    pid_t exitPid;
    pthread_t thread;

    printf("##### Forking - new process ######\n");
    printf("Forking\n");
    processId = fork();
    if(processId == 0) {
        allocate_memory("Child", allocBytes);
        print_memory_usage("Child");
        sleep_wrapper("Child", SHORT_SLEEP);
        printf("Child:\t exiting.\n");
        return 0;
    } else {
        print_memory_usage("Parent");
        printf("Parent:\t waiting for child to exit: %d\n", processId);
        exitPid = waitpid(processId, 0, 0);
        printf("Parent:\t sees child %d exit.\n", exitPid);
        print_memory_usage("Parent");
    }
    printf("##### Forking done ######\n\n");

    printf("##### Threading - new thread ######\n");
    printf("Threading\n");
    pthread_create(&thread, NULL, &thread_entry, NULL);
    print_memory_usage("Parent");
    sleep_wrapper("Parent", SHORT_SLEEP * 2);
    print_memory_usage("Parent");
    printf("Parent:\t waiting for child thread to join\n");
    pthread_join(thread, NULL);
    printf("Parent:\t sees child thread joining\n");
    print_memory_usage("Parent");
    printf("##### Threading done ######\n\n");

    return 0;
}