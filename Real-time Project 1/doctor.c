// this is the doctor process

#include "local.h"
int main(int argc, char *argv[])
{
    int create_Q(int, int);

    int max_patient_msg_wait_time = 5; //max time doc can wait for the patient's 'I am sick' response
    int shmid, semid, mid;
    union semun arg; // for semaphore operations

    // --------------------------- SHMEM variables
    MEMORY *memptr;
    // ---------------------------

    //usleep(1011000); // for simulation purposes

    //check number of arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s msq_id &\n", argv[0]);
        return 1;
    }
    // get the IPC key
    key_t key = atoi(argv[1]);

    // Access the semaphore set
    if ((semid = semget(key, 2, 0)) == -1)
    {
        perror("semget -- doctor -- access");
        exit(3);
    }
    arg.val = DOCTOR; // for semaphore operations

    //create the message queue for the doctor using his own id
    if ((mid = msgget(getpid(), IPC_CREAT | IPC_EXCL | 0666)) == -1)
    {
        puts("Msg Q Creation failed ");
    }
    printf("\033[1;32m"); //Set the text to the color GREEN
    printf("--->  I'm a doctor, my ID is %d and my Message Queue ID is %d\n", getpid(), mid);

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

    // ----------------------- write the Queue ID to the shared memory

    // acquire the semaphore
    if (semop(semid, &acquire, 1) == -1)
    {
        perror("semop ");
        exit(3);
    }

    int i = 0;
    while (memptr->doctors[i] != -1)
        i++;
    memptr->doctors[i] = mid;

    //release the semaphore
    if (semop(semid, &release, 1) == -1)
    {
        perror("semop ");
        exit(3);
    }

    // --------------------------------------------------------------------------------------


    while (1)
    {

        // acquire the semaphore
        if (semop(semid, &acquire, 1) == -1)
        {
            perror("semop ");
            exit(3);
        }

        //get the next patient on waiting list
        int i = 0;
        while (memptr->patients[i] == -1 && i < 100)
            i++;
        if (memptr->patients[i] == 0 || i == 1000)
        {
            //release the semaphore
            if (semop(semid, &release, 1) == -1)
            {
                perror("semop ");
                exit(3);
            }
            sleep(1);
            continue;
        }

        int patient_pid = memptr->patients[i]; //to store the pid of the patient the doctor currently works with

        printf("\033[1;32m"); //Set the text to the color GREEN
        printf("Doctor %d: Dealing with patient %d\n\n", getpid(), patient_pid);

        memptr->patients[i] = -1;

        //release the semaphore
        if (semop(semid, &release, 1) == -1)
        {
            perror("semop ");
            exit(3);
        }

        //send the patient sigusr1 signal
        kill(patient_pid, SIGUSR1);

        int secondsElapsed = 0; //since the doctor waits certain amount of time for patient response
        int msgrcv_flag = 0;    //1 if the doc received I am sick message from patient, 0 otherwise

        while (1)
        {
            if (msgrcv(mid, &message, sizeof(message), 1, IPC_NOWAIT) != -1)
            {
                msgrcv_flag = 1;
                printf("\033[1;32m"); //Set the text to the color GREEN
                printf("Doctor %d: Received 'I am sick' From Patient %d...\n\n", getpid(), message.sender_pid);
                break;
            }
            else
            {
                sleep(1);
                secondsElapsed++;
                if (secondsElapsed == max_patient_msg_wait_time)
                {
                    break;
                }
            }
        }

        //if the doctor didn't receive 'I am sick' message from the patient
        if (msgrcv_flag == 0)
        {
            continue;
        }
        //if received 'I am sick' and patient info message it reaches here

        sleep(2);
        message.mesg_type = 1;
        message.sender_pid = getpid();
        strcpy(message.mesg_text, "Show up");

        if (msgsnd(mid, &message, sizeof(message), 0) == -1)
            printf("Doctor %d: Message Send FAILED!\n", getpid());
    }


    // delete the message queue
    //msgctl(mid, IPC_RMID, (struct msqid_ds *) 0 );

    printf("\033[0m"); //Resets the text to default color
    return 0;
}
