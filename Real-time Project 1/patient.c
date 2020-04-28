// this is the patient  process
int threshhold; //  the threshold at which the parient dies

#include "local.h"
int max_doctor_msg_wait_time = 5;
static struct sigaction siga;

int main(int argc, char *argv[])
{
    // function declarations
    void signal_catcher(int, siginfo_t *, void *);
    void generate(INFO *);
    int determine(INFO *);
    int getRandom(int, int, int);
    // -------------------------------------------------------

    //ipcs IDs
    int shmid, semid;
    union semun arg; // for semaphore operations
    arg.val = PATIENT;
    // share memory pointer
    struct MEMORY *memptr;

    // -------------------------------------------------------

    INFO myInfo; // list of patient symptomps
    int severity;
    generate(&myInfo);             // generate random values for symptomps
    severity = determine(&myInfo); //determine severity based on info struct

    // -------------------------------------------------------

    //check number of arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s msq_id &\n", argv[0]);
        return 1;
    }
    // get the IPC key
    key_t key = atoi(argv[1]);

    siga.sa_sigaction = *signal_catcher;
    siga.sa_flags |= SA_SIGINFO; // get detail info

    // change signal action,
    if (sigaction(SIGUSR1, &siga, NULL) != 0)
    {
        printf("error sigaction()");
        return errno;
    }

    /*
   * Access the semaphore set
   */
    if ((semid = semget(key, 2, 0)) == -1)
    {
        perror("semget -- patient -- access");
        exit(3);
    }

    /*
     * Access, attach and reference the shared memory
    */
    if ((shmid = shmget(key, sizeof(MEMORY), 0)) != -1)
    {
        if ((memptr = shmat(shmid, NULL, 0)) == (MEMORY *)-1)
        {
            perror("shmat -- doctor -- attach");
            exit(1);
        }
    }
    else
    {
        perror("shmget -- doctor -- access");
        exit(2);
    }

    //now I will Aquire a semaphore , write the patient id in the shared memory and then release it
    // ----------------------- write the Queue ID to the shared memory

    // acquire the semaphore
    if (semop(semid, &acquire, 1) == -1)
    {
        perror("semop ");
        exit(3);
    }

    int i = 0;
    while (memptr->patients[i] != -1)
        i++;
    memptr->patients[i] = getpid();

    //release the semaphore
    if (semop(semid, &release, 1) == -1)
    {
        perror("semop ");
        exit(3);
    }

    // --------------------------------------------------------------------------------------

    printf("\033[1;31m"); //Set the text to the color RED
    printf("+ I'm a NEW Patient, my ID is %d Severity = %d  \n\n", getpid(), severity);

    while (1)
    {
        sleep(2);
        severity += 1;
        if (severity > threshhold)
        {
            printf("- Patient %d died \n\n", (int)getpid());
            kill(getppid(), SIGUSR2);
        }
    }

    printf("\033[0m"); //Resets the text to default color
    return 0;
}

void signal_catcher(int sig, siginfo_t *siginfo, void *context)
{
    printf("\033[1;31m"); //Set the text to the color GREEN
    printf("Patient %d: Received Signal %d", getpid(), sig);
    // get pid of sender,
    pid_t sender_pid = siginfo->si_pid;
    printf("\033[1;31m"); //Set the text to the color GREEN
    printf(" From Doctor [%d], Sending I am sick message\n\n\n", (int)sender_pid);
    int mid;
    mid = msgget((int)sender_pid, 0666 | IPC_CREAT);

    message.mesg_type = 1;
    message.sender_pid = getpid();
    strcpy(message.mesg_text, "I am sick");

    //SEND PATIENT INFO TO DOCTOR

    if (msgsnd(mid, &message, sizeof(message), 0) == -1)
        printf("Message Send FAILED!\n");

    int msgrcv_flag = 0;    //1 if the patient received show up message from doctor, 0 otherwise
    int secondsElapsed = 0; //since the patient waits certain amount of time for doctor's response

    while (1)
    {

        if (msgrcv(mid, &message, sizeof(message), 1, IPC_NOWAIT) != -1)
        {
            msgrcv_flag = 1;
            break;
        }
        else
        {
            sleep(1);
            secondsElapsed++;
            if (secondsElapsed == max_doctor_msg_wait_time)
            {
                break;
            }
        }
    }

    //if the patient didn't receive 'show up' message from the patient
    if (msgrcv_flag == 0)
    {
        printf("\033[1;31m"); //Set the text to the color GREEN
        printf("Patient %d: Didn't Receive 'show up' From Doctor...\n\n", getpid());
        printf("\033[1;31m"); //Set the text to the color GREEN
        printf("Patient %d: sending SIGUSR2 to PARENT \n\n", getpid());

        kill(getppid(), SIGUSR2);
        exit(-2); //-2 means dead
    }
    else
    {
        printf("\033[1;31m"); //Set the text to the color GREEN
        printf("Patient %d :  I have RECOVERED !\n\n", getpid());
        exit(0); //0 means recovered
    }

    return;
}

int getRandom(int lower, int upper, int flag)
{
    srand(time(NULL)); // seeding rand()
    int num = (rand() % (upper - lower + 1)) + lower;

    if (flag == 0)
    {

        if (num >= 4)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return num;
    }
}

// will determine severity based on the symptoms
int determine(INFO *myInfo)
{

    int severity = 0;
    if (myInfo->AGE >= 50)
    {
        severity += 1;
    }
    if (myInfo->COVID19_BREATH == 1)
    {
        severity++;
    }
    else if (myInfo->COVID19_BREATH == 1)
    {
        severity++;
    }
    else if (myInfo->COVID19_BREATH == 1)
    {
        severity++;
    }
    if (myInfo->BLOOD_HYPER == 1)
    {
        severity += 1;
    }
    if (myInfo->HEART_RESP == 1)
    {
        severity += 2;
    }
    if (myInfo->CANCER == 1)
    {
        severity += 3;
    }
    return severity;
}

void generate(INFO *myInfo)
{ // will generate values for the covid-19 symptoms
    FILE *in = fopen("input.txt", "r");
    char s[100];
    int lineCount = 1;
    while (fgets(s, 100, in) != NULL)
    {
        if (lineCount == 1 | lineCount == 2)
        {
            lineCount++;
            continue;
        }
        else
        {
            int i = 0;
            char *token = strtok(s, " ");
            int min;
            int max;

            while (token != NULL)
            {
                if (i == 0)
                {
                    i++;
                    continue;
                }
                else if (i == 2)
                    min = atoi(token);
                else if (i == 3)
                    max = atoi(token);
                i++;
                token = strtok(NULL, " ");
            }

            if (lineCount == 3)
                myInfo->AGE = getRandom(min, max, 1);
            if (lineCount == 4)
                myInfo->COVID19_FEVER = getRandom(min, max, 1);
            if (lineCount == 5)
                myInfo->COVID19_COUGH = getRandom(min, max, 1);
            if (lineCount == 6)
                myInfo->COVID19_BREATH = getRandom(min, max, 1);
            if (lineCount == 7)
                myInfo->BLOOD_HYPER = getRandom(min, max, 1);
            if (lineCount == 8)
                myInfo->HEART_RESP = getRandom(min, max, 1);
            if (lineCount == 9)
                myInfo->CANCER = getRandom(min, max, 1);
            if (lineCount == 10)
                threshhold = min;
        }
        lineCount++;
    }
    fclose(in);
}
