#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <sys/epoll.h>
#include "task.h"


#define PORT 8080
#define MAX_WORKERS 10
#define MAX_CLIENTS 1000
#define MAX_TASK_QUEUE 100
#define MAX_EVENTS 1024

typedef struct
{
    int socket;
    int active;
} Worker;

typedef struct
{
    int client_socket;
    Task task;
} TaskQueueItem;

Worker workers[MAX_WORKERS] = {0};
TaskQueueItem task_queue[MAX_TASK_QUEUE] = {0};
int task_queue_front = 0;
int task_queue_rear = 0; 
int worker_count = 0;
int current_worker_index = 0; 
time_t last_check_time = 0;

int is_task_queue_empty()
{
    return task_queue_front == task_queue_rear;
}

int is_task_queue_full()
{
    return (task_queue_rear + 1) % MAX_TASK_QUEUE == task_queue_front;
}

void enqueue_task(int client_socket, Task task)
{
    if (is_task_queue_full())
    {
        printf("Task queue is full. Dropping task.\n");
        close(client_socket);
        return;
    }
    task_queue[task_queue_rear].client_socket = client_socket;
    task_queue[task_queue_rear].task = task;
    task_queue_rear = (task_queue_rear + 1) % MAX_TASK_QUEUE;
    printf("Task enqueued. Queue size: %d\n", (task_queue_rear + MAX_TASK_QUEUE - task_queue_front) % MAX_TASK_QUEUE);
}

TaskQueueItem dequeue_task()
{
    TaskQueueItem task_item = task_queue[task_queue_front];
    task_queue_front = (task_queue_front + 1) % MAX_TASK_QUEUE;
    return task_item;
}

void remove_worker(int index)
{
    printf("Worker %d disconnected.\n", index + 1);
    close(workers[index].socket);
    workers[index].active = 0;
    workers[index] = workers[--worker_count]; 

    if (worker_count == 0)
    {
        current_worker_index = 0;
    }
    else if (current_worker_index >= worker_count)
    {
        current_worker_index = 0;
    }
}


int find_available_worker()
{
    if (worker_count == 0)
    {
        return -1;
    }

    int start_index = current_worker_index;
    do
    {
        if (workers[current_worker_index].active)
        {
            int selected_worker = current_worker_index;
            current_worker_index = (current_worker_index + 1) % worker_count; 
            return selected_worker;
        }
        current_worker_index = (current_worker_index + 1) % worker_count;
    } while (current_worker_index != start_index);

    return -1; 
}


void periodic_checker()
{
    for (int i = 0; i < worker_count; i++)
    {
        char heartbeat;
        int ret = recv(workers[i].socket, &heartbeat, sizeof(heartbeat), MSG_DONTWAIT);
        if (ret == 0 || (ret < 0 && errno != EWOULDBLOCK))
        {
            printf("Worker %d inactive or disconnected.\n", i + 1);
            remove_worker(i);
            i--; 
        }
    }
}

void process_task_queue()
{
    while (!is_task_queue_empty())
    {
        int worker_index = find_available_worker();
        if (worker_index >= 0)
        {
            TaskQueueItem task_item = dequeue_task();

            send(workers[worker_index].socket, &task_item.task, sizeof(Task), 0);
            printf("Task dequeued and sent to worker %d\n", worker_index + 1);

            long long result;
            int ret = recv(workers[worker_index].socket, &result, sizeof(result), 0);
            if (ret > 0)
            {
                send(task_item.client_socket, &result, sizeof(result), 0);
                printf("Result sent to client.\n");
                close(task_item.client_socket); 
            }
            else
            {
                printf("Worker %d failed to process dequeued task. Re-enqueuing.\n", worker_index + 1);
                enqueue_task(task_item.client_socket, task_item.task); 
            }
        }
        else
        {
            printf("No available workers. Stopping task processing temporarily.\n");
            break; 
        }
    }
}




int main()
{
    int server_fd, epoll_fd, new_socket;
    struct sockaddr_in address;
    struct epoll_event ev, events[MAX_EVENTS];
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Scheduler listening on port %d\n", PORT);

    if ((epoll_fd = epoll_create1(0)) < 0)
    {
        perror("Epoll create failed");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
    {
        perror("Epoll ctl failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < event_count; i++)
        {
            if (events[i].data.fd == server_fd)
            {
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                if (new_socket < 0)
                {
                    perror("Accept failed");
                    continue;
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) < 0)
                {
                    perror("Epoll ctl add failed");
                    close(new_socket);
                    continue;
                }
            }
            else
            {
                char type;
                int sock_fd = events[i].data.fd;
                int bytes_read = recv(sock_fd, &type, sizeof(type), MSG_DONTWAIT);

                if (bytes_read <= 0)
                {
                    close(sock_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, NULL);
                }
                else
                {
                    if (type == 'W')
                    {
                        if (worker_count >= MAX_WORKERS)
                        {
                            printf("Max workers reached. Disconnecting worker.\n");
                            close(sock_fd);
                        }
                        else
                        {
                            workers[worker_count++] = (Worker){.socket = sock_fd, .active = 1};
                            printf("Worker %d connected.\n", worker_count);
                            process_task_queue();
                        }
                    }
                    else if (type == 'C')
                    {
                        Task task;
                        recv(sock_fd, &task, sizeof(Task), 0);

                        int worker_index = find_available_worker();
                        if (worker_index >= 0 && is_task_queue_empty())
                        {
                            send(workers[worker_index].socket, &task, sizeof(Task), 0);
                            printf("Task sent to worker %d\n", worker_index + 1);

                            long long result;
                            int ret = recv(workers[worker_index].socket, &result, sizeof(result), 0);
                            if (ret > 0)
                            {
                                send(sock_fd, &result, sizeof(result), 0);
                                printf("Result sent to client.\n");
                            }
                            else
                            {
                                printf("Worker %d failed to process task. Task dropped.\n", worker_index + 1);
                            }
                            close(sock_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, NULL);
                        }
                        else
                        {
                            enqueue_task(sock_fd, task);
                        }
                    }
                }
            }
        }

        time_t current_time = time(NULL);
        if (current_time - last_check_time >= 5)
        {
            periodic_checker();
            process_task_queue();
            last_check_time = current_time;
        }
    }

    close(server_fd);
    return 0;
}
