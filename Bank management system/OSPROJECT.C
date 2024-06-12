#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // For sleep()
#include <signal.h>
#include <time.h> // For time()
#include <string.h> // For strcmp()

#define MAX_SIZE 50

// Structure to represent a bank Customer
typedef struct {
    int operation;
    char account_number[20];
} Customer;

// Structure to represent a queue
typedef struct {
    Customer items[MAX_SIZE];
    int front;
    int rear;
} Queue;

// Function forward declaration
void logCustomer(const char *account_number, const char *operation, int amount, const char *loan_status);
void sigint_handler(int signum);
void initQueue(Queue *queue);
int isEmpty(Queue *queue);
void enqueue(Queue *queue, Customer transaction);
Customer dequeue(Queue *queue);
void takeCustomerRating(const char *account_number);
void calculateAndDisplayAverageRating();
void *billPayment(void *arg);
void *cashWithdrawal(void *arg);
void *cashDeposit(void *arg);
void *loanApplication(void *arg);
void *currencyExchange(void *arg);
void *creditCardApplication(void *arg);
void *readCustomers(void *arg);
void *generateRandomCustomers();
void writeCustomerHistoryToFile();

// Global queues for each operation
Queue operation_queues[6]; // Updated for loan application
pthread_mutex_t mutexes[6]; // Updated for loan application
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for count variable
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for file access
pthread_cond_t file_cond = PTHREAD_COND_INITIALIZER; // Condition variable for file reading
int customer_count = 0; // Variable to keep track of the number of customers
int customer_served = 0;
int stop_reading = 0; // Flag to indicate whether to stop reading from file
long total_cash_deposited = 0; // Total cash deposited
long total_cash_withdrawn = 0; // Total cash withdrawn
long total_loan_amount_given = 0; // Total loan amount given
int cash_deposit_count = 0; // Number of cash deposits
int cash_withdrawal_count = 0; // Number of cash withdrawals
int loan_accepted_count = 0; // Number of loans accepted
int loan_rejected_count = 0; // Number of loans rejected
int bill_payment_count = 0; // Number of bill payments
int currency_exchange_count = 0;
int total_currency_exchanged = 0;
long total_credit_limit_given = 0;
int credit_card_application_count = 0;
long total_amount_collected = 0; // Total amount collected from bill payments

// Function to initialize a queue
void initQueue(Queue *queue)
{
    queue->front = -1;
    queue->rear = -1;
}

// Function to check if a queue is empty
int isEmpty(Queue *queue)
{
    return queue->rear == -1;
}

// Function to enqueue a transaction into a queue
void enqueue(Queue *queue, Customer transaction)
{
    if (queue->rear == MAX_SIZE - 1)
    {
        printf("Queue is full\n");
        return;
    }
    if (queue->front == -1)
        queue->front = 0;
    queue->rear++;
    queue->items[queue->rear] = transaction;
}

// Function to dequeue a transaction from a queue
Customer dequeue(Queue *queue)
{
    Customer item;
    if (isEmpty(queue))
    {
        printf("Queue is empty\n");
        item.operation = -1; // Indicates an empty transaction
        return item;
    }
    item = queue->items[queue->front];
    queue->front++;
    if (queue->front > queue->rear)
    {
        queue->front = -1;
        queue->rear = -1;
    }
    return item;
}


void takeCustomerRating(const char *account_number)
{
    int rating = 1 + rand() % 5;
    
    FILE *ratings_file = fopen("customer_ratings.txt", "a");
    if (ratings_file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(ratings_file, "%s %d\n", account_number, rating);
    fclose(ratings_file);
}
void calculateAndDisplayAverageRating() {
    FILE *ratings_file = fopen("customer_ratings.txt", "r");
    if (ratings_file == NULL) {
        perror("Error opening ratings file");
        exit(EXIT_FAILURE);
    }

    int total_ratings = 0;
    int total_score = 0;
    char account_number[20];
    int rating;

    while (fscanf(ratings_file, "%s %d", account_number, &rating) != EOF) {
        total_ratings++;
        total_score += rating;
    }

    fclose(ratings_file);

    if (total_ratings > 0) {
        double average_rating = (double)total_score / total_ratings;
        printf("Overall customer rating: %.2f\n", average_rating);
    } else {
        printf("No ratings recorded yet.\n");
    }
}
// Function for the bill payment operation
void *billPayment(void *arg)
{
    char *bills[3] = {"Electricity", "Water", "Gas"};
    while (1)
    {
        pthread_mutex_lock(&mutexes[0]); // Lock operation 1 queue
        if (!isEmpty(&operation_queues[0]))
        {
            char *bill = bills[rand() % 3];
            int amount = 2000 + rand() % 8001;
            Customer transaction = dequeue(&operation_queues[0]);
            printf("--------------------------------------------------\n");
            printf("Bank teller 1: Customer with account number %s is paying %s bill of amount %d\n", transaction.account_number, bill, amount);
            total_amount_collected += amount; // Update total amount collected
            bill_payment_count++; // Increment bill payment count
            sleep(rand() % 3 + 1); // Random delay between 1 to 3 seconds
            pthread_mutex_lock(&count_mutex); // Lock count variable
            customer_count--; // Decrement customer count after serving
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            pthread_mutex_unlock(&count_mutex); // Unlock count variable
            printf("Bank teller 1: Customer with account number %s is leaving\n", transaction.account_number);
            // Log transaction
            logCustomer(transaction.account_number, "bill payment", amount, "-");
            takeCustomerRating(transaction.account_number);
        }
        pthread_mutex_unlock(&mutexes[0]); // Unlock operation 1 queue

        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading && isEmpty(&operation_queues[0]))
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
    }
    pthread_exit(NULL);
}

// Function for the cash withdrawal operation
void *cashWithdrawal(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutexes[1]); // Lock operation 2 queue
        if (!isEmpty(&operation_queues[1]))
        {
            int amount = 1000 + rand() % 99001;
            Customer transaction = dequeue(&operation_queues[1]);
            printf("--------------------------------------------------\n");
            printf("Bank teller 2: Customer with account number %s is withdrawing %d PKR\n", transaction.account_number, amount);
            total_cash_withdrawn += amount; // Update total cash withdrawn
            cash_withdrawal_count++; // Increment cash withdrawal count
            sleep(rand() % 3 + 1); // Random delay between 1 to 3 seconds
            pthread_mutex_lock(&count_mutex); // Lock count variable
            customer_count--; // Decrement customer count after serving
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            pthread_mutex_unlock(&count_mutex); // Unlock count variable
            printf("Bank teller 2: Customer with account number %s is leaving\n", transaction.account_number);
            // Log transaction
            logCustomer(transaction.account_number, "cash withdrawal", amount, "-");
            takeCustomerRating(transaction.account_number);
        }
        pthread_mutex_unlock(&mutexes[1]); // Unlock operation 2 queue

        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading && isEmpty(&operation_queues[1]))
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
    }
    pthread_exit(NULL);
}

// Function for the cash deposit operation
void *cashDeposit(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutexes[2]); // Lock operation 3 queue
        if (!isEmpty(&operation_queues[2]))
        {
            int amount = 1000 + rand() % 99001;
            Customer transaction = dequeue(&operation_queues[2]);
            printf("--------------------------------------------------\n");
            printf("Bank teller 3: Customer with account number %s is depositing %d PKR\n", transaction.account_number, amount);
            total_cash_deposited += amount; // Update total cash deposited
            cash_deposit_count++; // Increment cash deposit count
            sleep(rand() % 3 + 1); // Random delay between 1 to 3 seconds
            pthread_mutex_lock(&count_mutex); // Lock count variable
            customer_count--; // Decrement customer count after serving
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            pthread_mutex_unlock(&count_mutex); // Unlock count variable
            printf("Bank teller 3: Customer with account number %s is leaving\n", transaction.account_number);
            // Log transaction
            logCustomer(transaction.account_number, "cash deposit", amount, "-");
            takeCustomerRating(transaction.account_number);
        }
        pthread_mutex_unlock(&mutexes[2]); // Unlock operation 3 queue

        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading && isEmpty(&operation_queues[2]))
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
    }
    pthread_exit(NULL);
}

// Function for the loan application operation
void *loanApplication(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutexes[3]); // Lock operation 4 queue
        if (!isEmpty(&operation_queues[3]))
        {
            // Generate random loan amount (between 50000 and 500000)
            long loan_amount = 10000 + rand() % 300001;
            // Generate random income (between 20000 and 200000)
            long income = 45000 + rand() % 200001;
            Customer transaction = dequeue(&operation_queues[3]);
            printf("--------------------------------------------------\n");
            printf("Bank teller 4: Customer with account number %s is applying for a loan of amount %ld PKR with income %ld PKR\n", transaction.account_number, loan_amount, income);
            // Loan approval criteria (income should be at least 3 times the loan amount)
            if (income >= 2 * loan_amount)
            {
                printf("Loan approved for customer with account number %s\n", transaction.account_number);
                total_loan_amount_given += loan_amount; // Update total loan amount given
                loan_accepted_count++; // Increment loan accepted count
                logCustomer(transaction.account_number, "loan application", loan_amount, "accepted"); // Log loan as accepted
                takeCustomerRating(transaction.account_number);
            }
            else
            {
                printf("Loan rejected for customer with account number %s\n", transaction.account_number);
                loan_rejected_count++; // Increment loan rejected count
                logCustomer(transaction.account_number, "loan application", loan_amount, "rejected"); // Log loan as rejected
                takeCustomerRating(transaction.account_number);
            }
            
            pthread_mutex_lock(&count_mutex); // Lock count variable
            customer_count--; // Decrement customer count after serving
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            pthread_mutex_unlock(&count_mutex); // Unlock count variable
            printf("Bank teller 4: Customer with account number %s is leaving\n", transaction.account_number);
        }
        pthread_mutex_unlock(&mutexes[3]); // Unlock operation 4 queue

        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading && isEmpty(&operation_queues[3]))
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
    }
    pthread_exit(NULL);
}


// Function for the currency exchange operation
void *currencyExchange(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutexes[4]); // Lock operation 5 queue
        if (!isEmpty(&operation_queues[4]))
        {
            // Generate random currency exchange amount (between 1000 and 100000)
            long exchange_amount = 1000 + rand() % 90001;
            Customer transaction = dequeue(&operation_queues[4]);
            printf("--------------------------------------------------\n");
            printf("Bank teller 5: Customer with account number %s is exchanging %ld PKR\n", transaction.account_number, exchange_amount);
            total_currency_exchanged += exchange_amount; // Update total currency exchanged
            currency_exchange_count++; // Increment currency exchange count
            sleep(rand() % 3 + 1); // Random delay between 1 to 3 seconds
            pthread_mutex_lock(&count_mutex); // Lock count variable
            customer_count--; // Decrement customer count after serving
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            pthread_mutex_unlock(&count_mutex); // Unlock count variable
            printf("Bank teller 5: Customer with account number %s is leaving\n", transaction.account_number);
            // Log transaction
            logCustomer(transaction.account_number, "currency exchange", exchange_amount, "-");
            takeCustomerRating(transaction.account_number);
        }
        pthread_mutex_unlock(&mutexes[4]); // Unlock operation 5 queue

        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading && isEmpty(&operation_queues[4]))
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
    }
    pthread_exit(NULL);
}

// Function for the credit card application operation
void *creditCardApplication(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutexes[5]); // Lock operation 6 queue
        if (!isEmpty(&operation_queues[5]))
        {
            // Generate random credit limit (between 50000 and 500000)
            long credit_limit = 50000 + rand() % 450001;
            Customer transaction = dequeue(&operation_queues[5]);
            printf("--------------------------------------------------\n");
            printf("Bank teller 6: Customer with account number %s is applying for a credit card with a limit of %ld PKR\n", transaction.account_number, credit_limit);
            total_credit_limit_given += credit_limit; // Update total credit limit given
            credit_card_application_count++; // Increment credit card application count
            sleep(rand() % 3 + 1); // Random delay between 1 to 3 seconds
            pthread_mutex_lock(&count_mutex); // Lock count variable
            customer_count--; // Decrement customer count after serving
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            pthread_mutex_unlock(&count_mutex); // Unlock count variable
            printf("Bank teller 6: Customer with account number %s is leaving\n", transaction.account_number);
            // Log transaction
            logCustomer(transaction.account_number, "credit card application", credit_limit, "-");
            takeCustomerRating(transaction.account_number);
        }
        pthread_mutex_unlock(&mutexes[5]); // Unlock operation 6 queue

        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading && isEmpty(&operation_queues[5]))
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
    }
    pthread_exit(NULL);
}


void *readCustomers(void *arg)
{
    FILE *file = fopen("customers.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    Customer transaction;
    while (1)
    {
        if (customer_count >= MAX_SIZE)
        {
            sleep(1);
        }
        pthread_mutex_lock(&file_mutex); // Lock file access
        if (stop_reading)
        {
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        int result = fscanf(file, "%d %s", &transaction.operation, transaction.account_number);
        if (result != 2)
        {
            stop_reading = 1; // Set flag to stop reading from file
            pthread_cond_signal(&file_cond); // Signal other threads that file reading is done
            pthread_mutex_unlock(&file_mutex); // Unlock file access
            break;
        }
        pthread_mutex_unlock(&file_mutex); // Unlock file access
        pthread_mutex_lock(&count_mutex); // Lock count variable
        if (customer_count < MAX_SIZE)
        {
            enqueue(&operation_queues[transaction.operation - 1], transaction);
            customer_served++;
            customer_count++;
            printf("\033[0;31m"); // Red color
	    printf("Customer count: %d\n", customer_count);
	    printf("\033[0m"); // Reset color to default
            if (customer_count == 1)
            {
                // Signal other threads waiting for transactions
                pthread_cond_broadcast(&file_cond);
            }
        }
        pthread_mutex_unlock(&count_mutex); // Unlock count variable
        usleep((rand() % 500000) + 500000);
    }
    fclose(file);
    // Signal other threads to stop
    stop_reading = 1;
    pthread_exit(NULL);
}

// Function to log transaction with operation type and loan status (if applicable)
void logCustomer(const char *account_number, const char *operation, int amount, const char *loan_status)
{
    FILE *log_file = fopen("customer_logs.txt", "a");
    if (log_file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    fprintf(log_file, "Date and Time: %s", asctime(timeinfo));
    fprintf(log_file, "Account Number: %s\n", account_number);
    fprintf(log_file, "Operation: %s\n", operation);
    fprintf(log_file, "Amount: %d PKR\n", amount);

    // Log loan status only if the operation is a loan application
    if (strcmp(operation, "loan application") == 0)
    {
        fprintf(log_file, "Loan Status: %s\n", loan_status);
    }

    fprintf(log_file, "--------------------------------------------------\n");

    fclose(log_file);
}

// Signal handler function for Ctrl+C (SIGINT)
void sigint_handler(int signum)
{
    printf("\nClosing Bank...\n");
    stop_reading = 1;
    pthread_cond_signal(&file_cond); // Signal the readCustomers function to stop
}

void *generateRandomCustomers()
{
    FILE *file = fopen("customers.txt", "w");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL)); // Seed the random number generator

    int total_transactions = 1000; // Generate 1000 transactions

    while (total_transactions > 0)
    {
        // Generate random operation (between 1 and 4) for loan application
        int operation = (rand() % 6) + 1;

        // Generate random 16-digit account number
        long long account_number = 0;
        for (int j = 0; j < 16; j++)
        {
            account_number = account_number * 10 + (rand() % 10);
        }

        // Write transaction to file
        fprintf(file, "%d\n%lld\n", operation, account_number);

        // Decrease the total number of transactions
        total_transactions--;
    }

    fclose(file);
    pthread_exit(NULL);
}

void writeCustomerHistoryToFile()
{
    FILE *history_file = fopen("transaction_history.txt", "a");
    if (history_file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    fprintf(history_file, "Date: %s", asctime(timeinfo));
    fprintf(history_file, "Number of cash deposits: %d\n", cash_deposit_count);
    fprintf(history_file, "Number of cash withdrawals: %d\n", cash_withdrawal_count);
    fprintf(history_file, "Number of loans accepted: %d\n", loan_accepted_count);
    fprintf(history_file, "Number of loans rejected: %d\n", loan_rejected_count);
    fprintf(history_file, "Number of bill payments: %d\n", bill_payment_count);
    fprintf(history_file, "Number of currency exchanges: %d\n", currency_exchange_count);
    fprintf(history_file, "Number of credit card applications: %d\n", credit_card_application_count);
    fprintf(history_file, "Total amount collected from bill payments: %ld PKR\n", total_amount_collected);
    fprintf(history_file, "Total cash deposited: %ld PKR\n", total_cash_deposited);
    fprintf(history_file, "Total cash withdrawn: %ld PKR\n", total_cash_withdrawn);
    fprintf(history_file, "Total loan amount given: %ld PKR\n", total_loan_amount_given);
    fprintf(history_file, "--------------------------------------------------\n");
    fclose(history_file);
}

int main()
{
    system("clear");
    printf("  _    __              _      _  _                                                   _      _ \n");
    printf(" | |  |  _ \\            | |    |  \\/  |                                                 | |    | |\n");
    printf(" | |  | |) | _ _ _ _ | | _ | \\  / | _ _ _ _   _ _  _ _  _ _ _ __   _ _ _ | |   | |\n");
    printf(" | |  |  _ < / ` | ' \\| |/ / | |\\/| |/ ` | ' \\ / ` |/ _` |/ _ \\ ' ` _ \\ / _ \\ '_ \\| __|  | |\n");
    printf(" | |  | |) | (| | | | |   <  | |  | | (| | | | | (| | (| |  _/ | | | | |  _/ | | | |   | |\n");
    printf(" | |  |_/ \\,|| |||\\\\ ||  ||\\_,|| ||\\_,|\\_, |\\_|| || ||\\_|| ||\\__|  | |\n");
    printf(" | |                                                      __/ |                                | |\n");
    printf(" ||                                __           _    |_/                                 |_|\n");
    printf(" | |                               / __|         | |                                         | |\n");
    printf(" | |                              | (_  _   _ _| |_ _ _ _ __                           | |\n");
    printf(" | |                               \\_ \\| | | / _| _/ _ \\ '_ ` _ \\                          | |\n");
    printf(" | |                               _) | || \\_ \\ ||  _/ | | | | |                         | |\n");
    printf(" | |                              |__/ \\, |_/\\\\_|| || ||                         | |\n");
    printf(" | |                                       __/ |                                               | |\n");
    printf(" ||                                      |_/                                                ||\n");


    printf("                                          _ .-'`-. _\n");
    printf("                                         ;.'____'.;\n");
    printf("                              ____n.[____].n____\n");
    printf("                             |\"\"\"\"\"\"\"\"||==||==||==||\"\"\"\"\"\"\"\n");
    printf("                             |\"\"\"\"\"\"\"\"\"\"\"\"||..||..||..||\"\"\"\"\"\"\"\"\"\"\"\"|\n");
    printf("                             |LI LI LI LI||LI||LI||LI||LI LI LI LI|\n");
    printf("                             |.. .. .. ..||..||..||..||.. .. .. ..|\n");
    printf("                             |LI LI LI LI||LI||LI||LI||LI LI LI LI|\n");
    printf("                          ,,;;,;;;,;;;,;;;,;;;,;;;,;;;,;;,;;;,;;;,;;,,\n");
    printf("                         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n");

    sleep(2);
    system("clear");
	    
    printf("----------------------------------------------------------------------------------------------------------------------------\n");
	
    printf("                      ___                 _                      _   _                                                   \n"
           "                     |  __ \\               | |                    | | | |                                                  \n"
           "                     | |  | | __   __| | _  _ _   __  _| | | |_  _   _                                         \n"
           "                     | |  | |/ _ \\ \\ / / _ \\ |/ _ \\| '_ \\ / _ \\/ ` | | ' \\| | | |                                        \n"
           "                     | |_| |  _/\\ V /  _/ | () | |) |  _/ (| | | |) | |_| |                                        \n"
           "                     |__/ \\_| \\/ \\_||\\_/| ./ \\_|\\,| |./ \\_, |                                        \n"
           "                                                   | |                        __/ |                                        \n"
           "                     _       _  _                 ||         _  __  _|_/    _  _   __   _ _  _ __             \n"
           "     /\\             | |     | |/ /                            / / |/ /_ \\|_ \\      | || | | __| / /| || |\\ \\            \n"
           "    /  \\   _ _  _| |_   | ' /_   _ _ _ __   _ _ _ _  | || ' /   ) |  ) |__| || || |_  / /| || |_| |           \n"
           "   / /\\ \\ | '_ \\/ _| ' \\  |  <| | | | '_ ` _ \\ / ` | '| | ||  <   / /  / /__|_   |_ \\| ' \\__   _| |           \n"
           "  / __ \\| | | \\_ \\ | | | | . \\ || | | | | | | (| | |    | || . \\ / / / /_         | |  _) | (_) | | | | |           \n"
           " //    \\\\| ||_/| || ||\\\_,|| || ||\\,||    | |||\\\\_|_|        || |_/ \\_/  || |_|           \n"
           "  _   _                                        _              \\\\       _    __  __  __        _  _ _ //_ __  \n"
           " | \\ | |                                 /\\   | |                      | |  / / |/ /_ \\|_ \\      | || |_ \\| __|_ \\ \\ \n"
           " |  \\| | _  _ _ __   _ _ _ _      /  \\  | |_  _ _ _   _  _| | | || ' /   ) |  ) |_| || | ) | |__    ) | |\n"
           " | . ` |/ _ \\| '_ ` _ \\ / ` | ' \\    / /\\ \\ | '_ \\| '_ ` _ \\ / _ \\/ ` | | ||  <   / /  / /__|_   / /|__ \\  / /| |\n"
           " | |\\  | () | | | | | | (| | | | |  / __ \\| | | | | | | | |  _/ (| | | || . \\ / /_ / /_         | |/ /_ _) |/ /_| |\n"
           " || \\|\\_/|| || ||\\,|| || //    \\\\| ||| || ||\\_|\\,| | |||\\\_|_|        ||_|_/|_| |\n\n");
    printf("    /\\    / |/ _|                 | |              / / |/ /_ \\|_ \\      | || | | || |_  | __\\ \\                   \n");
    printf("   /  \\  | || | _ _ _ _        | | _ _ _ _   | || ' /   ) |  ) |__| || || || |_  / /| |__  | |                  \n");
    printf("  / /\\ \\ |  |  _/ _` | ' \\   _   | |/ ` | ' \\  | ||  <   / /  / /__|_   _|_   |/ / |__ \\ | |                  \n");
    printf(" / __ \\| | | || (| | | | | | || | (| | | | | | || . \\ / /_ / /_         | |    | | / /   _) || |                  \n");
    printf("//    \\\\| ||\\_,|| ||  \\_/ \\,|| || | |||\\\\_|_|        ||    ||//   |_/ |_|                  \n");
    printf("                                                     \\\\                                             //                   \n");
    
    sleep(2);
    system("clear");
    
    pthread_t tid[8]; // Array to hold thread IDs
    int i;

    // Register signal handler for SIGINT (Ctrl+C)
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        perror("Error setting signal handler");
        exit(EXIT_FAILURE);
    }

    // Initialize queues and mutex locks
    for (i = 0; i < 6; i++)
    {
        initQueue(&operation_queues[i]);
        pthread_mutex_init(&mutexes[i], NULL);
    }

    // Create thread to generate random transactions
    pthread_create(&tid[7], NULL, generateRandomCustomers, NULL);

    // Wait for a short time to ensure file is created before reading
    sleep(1);

    // Create thread to read transactions from file
    pthread_create(&tid[6], NULL, readCustomers, NULL);

    // Create threads for each operation
    pthread_create(&tid[0], NULL, billPayment, NULL);
    pthread_create(&tid[1], NULL, cashWithdrawal, NULL);
    pthread_create(&tid[2], NULL, cashDeposit, NULL);
    pthread_create(&tid[3], NULL, loanApplication, NULL); // Thread for loan application
    pthread_create(&tid[4], NULL, currencyExchange, NULL); // Thread for currency exchange
    pthread_create(&tid[5], NULL, creditCardApplication, NULL); // Thread for credit card application

    // Wait for file reading thread to finish
    pthread_join(tid[6], NULL);

    // Signal other threads to stop
    stop_reading = 1;

    // Wait for all threads to finish
    for (i = 0; i < 8; i++)
    {
    	if(i==6)
    	{
    		continue;
    	}
        pthread_join(tid[i], NULL);
    }

    // Write transaction history to file
    writeCustomerHistoryToFile();

    // Destroy mutex locks
    for (i = 0; i < 6; i++)
    {
        pthread_mutex_destroy(&mutexes[i]);
    }

    // Print summary
    printf("\nBank Closed\n");
    printf("--------------------------------------------------\n");
    printf("History of the day:\n");
    printf("--------------------------------------------------\n");
    printf("Number of customers served: %d\n", customer_served);
    printf("Number of cash deposits: %d\n", cash_deposit_count);
    printf("Number of cash withdrawals: %d\n", cash_withdrawal_count);
    printf("Number of loans accepted: %d\n", loan_accepted_count);
    printf("Number of loans rejected: %d\n", loan_rejected_count);
    printf("Number of bill payments: %d\n", bill_payment_count);
    printf("Number of currency exchanges: %d\n", currency_exchange_count);
    printf("Number of credit card applications: %d\n", credit_card_application_count);
    printf("Total amount collected from bill payments: %ld PKR\n", total_amount_collected);
    printf("Total cash deposited: %ld PKR\n", total_cash_deposited);
    printf("Total cash withdrawn: %ld PKR\n", total_cash_withdrawn);
    printf("Total loan amount given: %ld PKR\n", total_loan_amount_given);
    
    calculateAndDisplayAverageRating();
    
    return 0;
}