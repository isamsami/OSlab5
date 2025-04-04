#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

/* these may be any values >= 0 */
#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
/* the available amount of each resource */
int available[NUMBER_OF_RESOURCES];
/* the maximum demand of each customer */
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
/* the amount currently allocated to each customer *
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
/* the remaining need of each customer */
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
/* mutex to protect the shared data structures */
pthread_mutex_t mutex;

// function to check if the system is in a safe state
bool is_safe() {
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS];
    
    // Initialize work and finish arrays
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i];
    }
    
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        finish[i] = false;
    }
    
    // Find a customer whose needs can be satisfied
    bool found;
    do {
        found = false;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                // Check if customer's needs can be satisfied
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        break;
                    }
                }
                
                if (j == NUMBER_OF_RESOURCES) {
                    // All needs can be satisfied
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++) {
                        work[k] += allocation[i][k];
                    }
                    finish[i] = true;
                    found = true;
                }
            }
        }
    } while (found);
    
    // Check if all customers are finished
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i]) {
            return false;
        }
    }
    
    return true;
}

// request resources 
int request_resources(int customer_num, int request[]) {
    // Check if request is valid
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            printf("Customer %d requested more than its maximum need\n", customer_num);
            return -1;
        }
        if (request[i] > available[i]) {
            printf("Customer %d must wait, resources not available\n", customer_num);
            return -1;
        }
    }
    
    // Try to allocate resources
    pthread_mutex_lock(&mutex);
    
    // Temporarily allocate resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }
    
    // Check if system is still in safe state
    if (is_safe()) {
        pthread_mutex_unlock(&mutex);
        printf("Customer %d: Resource request granted\n", customer_num);
        return 0;
    } else {
        // If not safe, rollback allocation
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&mutex);
        printf("Customer %d: Resource request denied (would lead to unsafe state)\n", customer_num);
        return -1;
    }
}

// release resources 
int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);
    
    // Check if release is valid
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (release[i] > allocation[customer_num][i]) {
            pthread_mutex_unlock(&mutex);
            printf("Customer %d tried to release more resources than allocated\n", customer_num);
            return -1;
        }
    }
    
    // Release resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }
    
    pthread_mutex_unlock(&mutex);
    printf("Customer %d: Resources released successfully\n", customer_num);
    return 0;
}

// Customer thread function 
void* customer_thread(void* arg) {
    int customer_num = *(int*)arg;
    int request[NUMBER_OF_RESOURCES];
    int release[NUMBER_OF_RESOURCES];
    
    srand(time(NULL) + customer_num); // Unique seed for each customer
    
    // Loop continuously, making requests and releases
    while (1) {
        // Generate random request
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            if (need[customer_num][i] > 0) {
                request[i] = rand() % (need[customer_num][i] + 1);
            } else {
                request[i] = 0;
            }
        }
        
        // Request resources
        if (request_resources(customer_num, request) == 0) {
            // If granted, use the resources (sleep)
            sleep(rand() % 3 + 1);
            
            // Generate random release
            for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
                if (allocation[customer_num][i] > 0) {
                    release[i] = rand() % (allocation[customer_num][i] + 1);
                } else {
                    release[i] = 0;
                }
            }
            
            // Release resources
            release_resources(customer_num, release);
        }
        
        // Sleep before next request
        sleep(rand() % 3 + 1);
    }
    
    return NULL;
}

// print current system state
void print_state() {
    printf("\nCurrent System State:\n");
    printf("Available Resources: ");
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        printf("%d ", available[i]);
    }
    printf("\n\n");
    
    printf("Maximum Demand:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d ", maximum[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    printf("Current Allocation:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d ", allocation[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    
    printf("Remaining Need:\n");
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Customer %d: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Initialize data structures
void initialize(int argc, char* argv[]) {
    // Initialize available resources from command line arguments
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }
    
    // Initialize maximum demand randomly
    srand(time(NULL));
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            // Maximum demand should not exceed the total resources available
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;  // Initially no resources are allocated
            need[i][j] = maximum[i][j];  // Initially need = maximum
        }
    }
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != NUMBER_OF_RESOURCES + 1) {
        printf("Usage: %s <R1> <R2> ... <Rm>\n", argv[0]);
        printf("Where <Ri> is the number of instances of resource type i\n");
        return 1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Mutex initialization failed\n");
        return 1;
    }
    
    // Initialize data structures
    initialize(argc, argv);
    
    // Print initial state
    printf("Initial State:\n");
    print_state();
    
    // Create customer threads
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    int customer_ids[NUMBER_OF_CUSTOMERS];
    
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        customer_ids[i] = i;
        if (pthread_create(&customers[i], NULL, customer_thread, &customer_ids[i]) != 0) {
            printf("Failed to create customer thread %d\n", i);
            return 1;
        }
    }
    
    // Main thread can periodically print system state
    for (int i = 0; i < 10; i++) {
        sleep(5);
        pthread_mutex_lock(&mutex);
        print_state();
        pthread_mutex_unlock(&mutex);
    }
    
    // In a real implementation, we would have some way to end the program
    // Here we'll just cancel the threads after some time
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_cancel(customers[i]);
        pthread_join(customers[i], NULL);
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
