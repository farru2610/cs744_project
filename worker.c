#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include "task.h"

#define PORT 8080
#define THREAD_POOL_SIZE 4
#define TASK_QUEUE_SIZE 10

typedef struct {
    Task task;
    int sock;
} TaskData;

TaskData task_queue[TASK_QUEUE_SIZE];
int queue_front = 0, queue_rear = 0;
int task_count = 0;

pthread_mutex_t queue_mutex;
pthread_cond_t task_available;
pthread_cond_t queue_space_available;

pthread_t thread_pool[THREAD_POOL_SIZE];

void process_task(TaskData task_data) {
    Task task = task_data.task;
    int sock = task_data.sock;
    int num = task.data;

    long long result = 1;
    for (int i = 1; i <= num; i++) {
        result *= i;
    }
    printf("Factorial of %d is %lld\n", num, result);
    // sleep(2);

    pthread_mutex_lock(&queue_mutex); 
    send(sock, &result, sizeof(result), 0);
    pthread_mutex_unlock(&queue_mutex);
    printf("Sent result to scheduler: %lld\n", result);
}

void *worker_thread(void *arg) {
    while (true) {
        TaskData task_data;

        pthread_mutex_lock(&queue_mutex);
        while (task_count == 0) {
            pthread_cond_wait(&task_available, &queue_mutex);
        }

        task_data = task_queue[queue_front];
        queue_front = (queue_front + 1) % TASK_QUEUE_SIZE;
        task_count--;

        pthread_cond_signal(&queue_space_available);
        pthread_mutex_unlock(&queue_mutex);

        process_task(task_data);
    }
    return NULL;
}

void enqueue_task(TaskData task_data) {
    pthread_mutex_lock(&queue_mutex);
    while (task_count == TASK_QUEUE_SIZE) {
        pthread_cond_wait(&queue_space_available, &queue_mutex);
    }

    task_queue[queue_rear] = task_data;
    queue_rear = (queue_rear + 1) % TASK_QUEUE_SIZE;
    task_count++;

    pthread_cond_signal(&task_available);
    pthread_mutex_unlock(&queue_mutex);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char type = 'W';

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&task_available, NULL);
    pthread_cond_init(&queue_space_available, NULL);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&thread_pool[i], NULL, worker_thread, NULL) != 0) {
            perror("Failed to create thread pool");
            return -1;
        }
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    printf("Connected to Scheduler...\n");
    send(sock, &type, sizeof(type), 0);

    while (1) {
        Task task;
        if (recv(sock, &task, sizeof(Task), 0) > 0) {
            printf("Received task from Scheduler: %d\n", task.data);

            TaskData task_data;
            task_data.task = task;
            task_data.sock = sock;

            enqueue_task(task_data);
        }
    }

    close(sock);

    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&task_available);
    pthread_cond_destroy(&queue_space_available);

    return 0;
}
