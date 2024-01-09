#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#ifdef __APPLE__
    #include <mach/mach.h>
#endif

#define SHORT_SLEEP 1
#define LONG_SLEEP 3

size_t allocBytes = 1024 * 1024 * 100;
int cleanup = 0;

void print_memory_usage(char *id) {
    long rss = 0;
    #ifdef __APPLE__
        struct task_basic_info t_info;
        mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
        task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
        rss = t_info.resident_size;
    #elif defined __linux__
        FILE* fd;
        char token1[32];
        char token2[32];
        long pageSize = sysconf(_SC_PAGESIZE);
        fd = fopen("/proc/self/statm", "r");
        fscanf(fd, "%31s %31s", token1, token2);
        rss = atoi(token2) * pageSize;
    #endif
    printf("%s:\t using memory %ld\n", id, rss);
}

void* allocate_memory(char *id, size_t length) {
    char *allocatedMemory;
    printf("%s\t allocating memory\n", id);
    allocatedMemory = mmap(0, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    memset(allocatedMemory, 42, length);
    return allocatedMemory;
}

void deallocate_memory(char *id, void *mem, size_t length) {
    if(!cleanup) return;
    printf("%s:\t deallocating memory\n", id);
    munmap(mem, length);
}

void sleep_wrapper(char *id, int duration) {
    printf("%s:\t sleeping for %d seconds\n", id, duration);
    sleep(duration);
}

void* thread_entry(void *arg) {
    void *mem;
    printf("Thread:\t entry\n");
    sleep_wrapper("Thread", SHORT_SLEEP);
    mem = allocate_memory("Thread", allocBytes);
    print_memory_usage("Thread");
    sleep_wrapper("Thread", LONG_SLEEP);
    deallocate_memory("Thread", mem, allocBytes);
    printf("Thread:\t exit\n");
    return NULL;
}

int main(int argc, char **argv) {
    pid_t processId;
    pid_t exitPid;
    pthread_t thread;
    int opt;
    int ch;

    // free memory before exiting the thread when called with -c
    while ((ch = getopt(argc, argv, "c")) != -1) {
        switch (ch) {
            case 'c':
                cleanup = 1;
                break;
            case '?':
            default:
                return 0;
            }
    }
    argc -= optind;

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