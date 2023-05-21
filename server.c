#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_PAIRS 10000 // The maximum number of key-value pairs
#define MAX_STRLEN 256  // The maximum length of key and value strings
#define MAX_FOLLOWERS 2 // The maximum number of followers

/* KVS structure */
typedef struct
{
    char key[MAX_STRLEN];
    char value[MAX_STRLEN];
} KeyValuePair;

KeyValuePair kv_store[MAX_PAIRS];
int kv_count = 0;

typedef struct
{
    struct sockaddr_in addr;
    socklen_t addr_len;
} Follower;

Follower followers[MAX_FOLLOWERS];
int follower_count = 2;

// Function to put a key-value pair into the dictionary
void put(const char *key, const char *value)
{
    int i;
    for (i = 0; i < kv_count; i++)
    {
        if (strcmp(kv_store[i].key, key) == 0)
        {
            strcpy(kv_store[i].value, value);
            printf("PUT is done\n");
            return;
        }
    }

    if (kv_count < MAX_PAIRS)
    {
        strcpy(kv_store[kv_count].key, key);
        strcpy(kv_store[kv_count].value, value);
        kv_count++;
        printf("PUT is done\n");
    }
}

// Function to get the value for a key from the dictionary
const char *get(const char *key)
{
    int i;
    for (i = 0; i < kv_count; i++)
    {
        if (strcmp(kv_store[i].key, key) == 0)
        {
            printf("GET is done\n");
            return kv_store[i].value;
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Input: %s port_number role\n", argv[0]);
        return 1;
    }

    int SERVER_PORT = atoi(argv[1]);
    int FOLLOWER_PORTS[MAX_FOLLOWERS] = {5002, 5003};
    int role = atoi(argv[2]);

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Could not create listen socket\n");
        exit(1);
    }

    if ((bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr))) < 0)
    {
        printf("Could not bind socket\n");
        exit(1);
    }

    struct sockaddr_in cli_addr;
    int cli_addr_len = sizeof(cli_addr);

    int maxlen = 1024;
    int n = 0;
    char RecvBuffer[maxlen];
    char RecvACKBuffer[maxlen];
    char PutCommandBuffer[maxlen];
    char SendBuffer[maxlen];

    int follower_sock;
    struct sockaddr_in follower_addr;
    socklen_t follower_addr_len = sizeof(follower_addr);

    if (role == 1) // Leader
    {
        // Create a new socket for followers
        if ((follower_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            printf("Could not create follower socket\n");
            exit(1);
        }

        memset(&follower_addr, 0, sizeof(follower_addr));
        follower_addr.sin_family = AF_INET;
        follower_addr.sin_port = htons(5005);
        follower_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        for (int i = 0; i < 2; i++)
        {
            struct sockaddr_in follower;
            follower.sin_family = AF_INET;
            follower.sin_port = htons(FOLLOWER_PORTS[i]);
            followers[i].addr = follower;
            followers[i].addr_len = sizeof(followers[i].addr);
        }
    }
    while (1)
    {
        n = recvfrom(sock, &RecvBuffer, sizeof(RecvBuffer), 0, (struct sockaddr *)&cli_addr, &cli_addr_len);
        if (n > 0)
        {
            RecvBuffer[n] = '\0';
            printf("%s\n", RecvBuffer);
            strcpy(PutCommandBuffer, RecvBuffer);
            char *ptr;
            char command[MAX_STRLEN];
            ptr = strtok(RecvBuffer, " ");
            strcpy(command, ptr);

            if (strcmp(command, "get") == 0)
            {
                if (role == 1) // Leader
                {
                    int i = 0;
                    char *str[2] = {0};
                    while (ptr != NULL)
                    {
                        if (i == 2)
                        {
                            break;
                        }
                        ptr = strtok(NULL, " ");
                        if (ptr == NULL)
                        {
                            break;
                        }
                        str[i] = (char *)malloc(strlen(ptr) + 1);
                        strcpy(str[i], ptr);
                        i++;
                    }
                    if (i != 1)
                    {
                        strcpy(SendBuffer, "Invalid command");
                    }
                    else
                    {
                        str[0][strcspn(str[0], "\n")] = '\0';
                        const char *value;
                        value = get(str[0]);
                        if (value == NULL)
                        {
                            strcpy(SendBuffer, "Key not found");
                            printf("Key not found\n");
                        }
                        else
                        {
                            strcpy(SendBuffer, value);
                        }
                    }
                    for (i = 0; str[i] != NULL; i++)
                    {
                        free(str[i]);
                    }
                    sendto(sock, &SendBuffer, sizeof(SendBuffer), MSG_DONTWAIT, (struct sockaddr *)&(cli_addr), sizeof(cli_addr));
                }
            }
            else if (strcmp(command, "put") == 0)
            {
                if (role == 1) // Leader
                {
                    int i = 0;
                    char *str[3] = {0};
                    while (ptr != NULL)
                    {
                        if (i == 3)
                        {
                            break;
                        }
                        ptr = strtok(NULL, " ");
                        if (ptr == NULL)
                        {
                            break;
                        }
                        str[i] = (char *)malloc(strlen(ptr) + 1);
                        strcpy(str[i], ptr);
                        i++;
                    }
                    if (i != 2)
                    {
                        strcpy(SendBuffer, "Invalid command");
                    }
                    else
                    {
                        // Propagate PUT command to followers
                        for (int j = 0; j < follower_count; j++)
                        {
                            sendto(follower_sock, PutCommandBuffer, sizeof(PutCommandBuffer), MSG_DONTWAIT, (struct sockaddr *)&(followers[j].addr), followers[j].addr_len);
                            int ack_n = recvfrom(follower_sock, &RecvACKBuffer, sizeof(RecvACKBuffer), MSG_DONTWAIT, (struct sockaddr *)&(followers[j].addr), &followers[j].addr_len);
                            if (ack_n > 0)
                            {
                                RecvACKBuffer[ack_n] = '\0';
                            }
                        }
                        // Update leader's value
                        put(str[0], str[1]);
                        strcpy(SendBuffer, "OK");
                    }
                    for (i = 0; str[i] != NULL; i++)
                    {
                        free(str[i]);
                    }
                    sendto(sock, &SendBuffer, sizeof(SendBuffer), MSG_DONTWAIT, (struct sockaddr *)&(cli_addr), sizeof(cli_addr));
                }
                else if (role == 0) // Follower
                {
                    int i = 0;
                    char *str[3] = {0};
                    while (ptr != NULL)
                    {
                        if (i == 3)
                        {
                            break;
                        }
                        ptr = strtok(NULL, " ");
                        if (ptr == NULL)
                        {
                            break;
                        }
                        str[i] = (char *)malloc(strlen(ptr) + 1);
                        strcpy(str[i], ptr);
                        i++;
                    }
                    if (i != 2)
                    {
                        strcpy(SendBuffer, "Invalid command");
                    }
                    else
                    {
                        // Update follower's value
                        put(str[0], str[1]);
                        strcpy(SendBuffer, "ACK");
                    }
                    for (i = 0; str[i] != NULL; i++)
                    {
                        free(str[i]);
                    }
                    sendto(sock, &SendBuffer, sizeof(SendBuffer), MSG_DONTWAIT, (struct sockaddr *)&(cli_addr), sizeof(cli_addr));
                }
            }
            else
            {
                strcpy(SendBuffer, "Invalid command");
                sendto(sock, &SendBuffer, sizeof(SendBuffer), MSG_DONTWAIT, (struct sockaddr *)&(cli_addr), sizeof(cli_addr));
                printf("%s\n", SendBuffer);
            }
        }
    }

    close(sock);
    close(follower_sock);
    return 0;
}
