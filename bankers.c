#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {0};
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_mutex_t mutex;

/* Function prototypes */
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
int is_safe_state();
void *customer_thread(void *arg);

/* Banker's Algorithm - Request Resources */
int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&lock);

    // Check if request is within need
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            pthread_mutex_unlock(&lock);
            return -1; // Request exceeds need
        }
    }

    // Check if enough resources are available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&lock);
            return -1; // Not enough resources available
        }
    }

    // Tentatively allocate resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    // Check for a safe state
    if (!is_safe_state()) {
        // Rollback if not safe
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&lock);
        return -1;
    }

    pthread_mutex_unlock(&lock);
    return 0; // Request granted
}

/* Release Resources */
int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&lock);

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        allocation[customer_num][i] -= release[i];
        available[i] += release[i];
        need[customer_num][i] += release[i];
    }

    pthread_mutex_unlock(&lock);
    return 0;
}

/* Safety Algorithm */
int is_safe_state() {
    int work[NUMBER_OF_RESOURCES];
    int finish[NUMBER_OF_CUSTOMERS] = {0};

    // Copy available to work
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        work[i] = available[i];

    int done;
    do {
        done = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int can_allocate = 1;
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        can_allocate = 0;
                        break;
                    }
                }
                if (can_allocate) {
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                        work[j] += allocation[i][j];
                    finish[i] = 1;
                    done = 1;
                }
            }
        }
    } while (done);

    // Check if all processes could finish
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        if (!finish[i])
            return 0;

    return 1;
}

/* Customer Thread */
void *customer_thread(void *arg) {
    int customer_num = *(int *)arg;
    int request[NUMBER_OF_RESOURCES];

    while (1) {
        sleep(rand() % 3 + 1); // Random delay

        // Generate a random request within the need limits
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
            request[i] = rand() % (need[customer_num][i] + 1);

        if (request_resources(customer_num, request) == 0) {
            printf("Customer %d granted resources.\n", customer_num);
            sleep(rand() % 3 + 1); // Hold resources for a while
            release_resources(customer_num, request);
            printf("Customer %d released resources.\n", customer_num);
        } else {
            printf("Customer %d denied resources.\n", customer_num);
        }
    }
}

/* Main Function */
int main(int argc, char *argv[]) {
    if (argc != NUMBER_OF_RESOURCES + 1) {
        printf("Usage: %s <R1> <R2> <R3>\n", argv[0]);
        return 1;
    }

    // Initialize available resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        available[i] = atoi(argv[i + 1]);

    // Randomly initialize max demand for each customer
    srand(time(NULL));
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            need[i][j] = maximum[i][j];
        }
    }

    pthread_mutex_init(&lock, NULL);

    // Create customer threads
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    int customer_ids[NUMBER_OF_CUSTOMERS];

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        customer_ids[i] = i;
        pthread_create(&customers[i], NULL, customer_thread, &customer_ids[i]);
    }

    // Join threads (this keeps the main thread running)
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        pthread_join(customers[i], NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
// Compile with: gcc -o bankers bankers.c -lpthread
// Run with: ./bankers <R1> <R2> <R3>