// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "task.h"

#define PORT 8080

int main(int argc, char *argv[]) {
    if(argc !=2){
        printf("Usage: %s <number>\n", argv[0]);
        return 1;
    }
    int sock = 0;
    struct sockaddr_in serv_addr;
    char type = 'C';

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    send(sock, &type, sizeof(type), 0);
    Task task;

    //strcpy(task.operation, "fact");
    //task.size = 1;
    task.data = atoi(argv[1]); 

    send(sock, &task, sizeof(Task), 0);
    printf("Factorial task of num = %d sent to scheduler\n", task.data);
    long long res;

    recv(sock, &res, sizeof(res), 0);
    if(res==0){
        printf("Task not performed\n");
        return 1;
    }
    printf("Result Recieved from Scheduler (Fact of %d): %lld\n", task.data, res);
    close(sock);
    return 0;
}
