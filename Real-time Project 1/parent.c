// the Parent Process
#include "local.h"

void signal_usr2_catcher(int, siginfo_t *, void *);
void exitt();

static struct sigaction siga;
key_t key;             // Inter Process Communication key
char m_key[10];        // ipc key passed with execlp
const int doc_num = 5; // number of doctors
pid_t doc_ids[doc_num], doc_id, p_id;
int shmid, semid;                    //ids for the shared memory and the semaphore set
static ushort sem_array[2] = {1, 1}; // semaphore initial values
union semun arg;                     // for semaphore operations
int fork_period;                 // time to fork a new patient
int deceased_patients = 0;
int main()
{
    FILE *in = fopen("input.txt", "r");
    char s[100];
    int lineCount = 1;
    while (fgets(s, 100, in) != NULL)
    {
        if (lineCount == 1)
        {
            lineCount++;
            continue;
        }

        else if (lineCount == 2)
        {
            int i = 0;
            char *token = strtok(s, " ");
            while (token != NULL)
            {
                if (i == 0)
                {
                    i++;
                    continue;
                }
                else if (i == 2)
                    fork_period = atoi(token);
                i++;
                token = strtok(NULL, " ");
            }
            break;
        }
    }
    fclose(in);
    siga.sa_sigaction = *signal_usr2_catcher;
    siga.sa_flags |= SA_SIGINFO; // get detail info

    // change signal action,
    if (sigaction(SIGUSR2, &siga, NULL) != 0)
    {
        printf("error sigaction()");
        return errno;
    }

    //shared memory variables

    MEMORY *memptr;
    static MEMORY memory;

    memptr = &memory;
    //------------ initialize shared memory
    for (int i = 0; i < 100; i++)
        memory.doctors[i] = -1; //initilize doctor msg q ids
    for (int i = 0; i < 100; i++)
        memory.patients[i] = -1; // initialize patient ids

    //----------------------------
    system("clear");
    printf("\033[1;34m");
    printf("\t\t\t    CORONA VIRUS SIMULATION\n\n");
    printf("Project By :  Adham Tamimi    Waleed Swaileh     Ezz Abuassab \n\n");
    sleep(2);
    puts("-------------------------------------------------------------------");
    printf(" Simulation Starts in 3 ...\n ");
    sleep(1);
    printf("2 ...\n ");
    sleep(1);
    printf("1 ...\n ");
    sleep(1);

    system("clear");
    printf("\033[1;31m"); //Set the text to the color RED
    printf("------------------- Patient \n\n");
    printf("\033[1;32m"); //Set the text to the color GREEN
    printf(" ------------------- Doctor\n\n");
    printf("\033[01;33m"); //Set the text to the color yellow
    printf(" ------------------- Parent\n\n");

    sleep(2);
    //---------------------------- Code Starts here

    // create an IPC key
    if ((key = ftok(".", 10)) == -1)
    {
        perror("Client: key generation");
        return 1;
    }

    //create a semaphore set
    if ((semid = semget(key, 2, IPC_CREAT | 0666)) != -1)
    {
        printf("\033[01;33m"); //Set the text to the color CYAN
        printf("PARENT :I'm the parent and I have created a semaphore set  with ID = %d\n", semid);
        arg.array = sem_array;
        semctl(semid, 0, SETALL, arg); // initialize semaphore values
    }

    //-----------------------------------------------------------
    /*
   * Create, attach and initialize the memory segment
   */
    //create
    if ((shmid = shmget(key, sizeof(memory), IPC_CREAT | 0666)) != -1)
    {

        // attach
        if ((memptr = shmat(shmid, NULL, 0)) == (MEMORY *)-1)
        {
            perror("shmptr -- parent -- attach");
            exit(1);
        }
        // copy memory to shared memory
        memcpy(memptr, (struct MEMORY *)&memory, sizeof(memory));
    }

    printf("\033[01;33m"); //Set the text to the color CYAN
    printf("PARENT :I'm the parent and I have created a shmem set  with ID = %d\n", shmid);
    puts("");
    //-----------------------------------------------------------

    puts("-------------------------------------------------------------------");
    // forking doctors:
    for (int i = 0; i < doc_num; i++)
    {
        doc_id = fork();
        switch (doc_id)
        {
        case -1:
            puts("Doctor Fork Failure ");
            break;
        case 0: //success Doctor (child)
            sprintf(m_key, "%d", key);
            execlp("./doctor", "doctor", m_key, "&", 0);
            break;
        default: // success parent
            doc_ids[i] = doc_id;
            printf("\033[01;33m"); //Set the text to the color yellow
            printf("PARENT :I'm the parent and I have forked a doctor with ID = %d\n", doc_id);

            break;
        }
        usleep(110000); // this delay is needed because some doctors had the same msg q id
    }
    puts("-------------------------------------------------------------------");
    sleep(2);
    /*
    //print contents of shared memory
    puts("DOCTORS");
    for (int i = 0; i < 100; i++)
    {
        if (memptr->doctors[i] != -1)
            printf("%d\n", memptr->doctors[i]);
        else
            continue;
    }*/

    //parent Operation
    while (1)
    {

        sleep(fork_period);
        p_id = fork(); // fork a new patient
        sprintf(m_key, "%d", key);
        if (p_id == 0)
            execlp("./patient", "patient", m_key, "&", 0);
        /*   
        //print contents of shared memory
        puts("PATIENS:");
        for (int i = 0; i < 100; i++)
        {
            if (memptr->patients[i] != -1)
                printf("%d\n", memptr->patients[i]);
            else
                continue;
        }*/
    }
    return 0;
}

void signal_usr2_catcher(int sig, siginfo_t *siginfo, void *context)
{

    printf("\033[01;33m"); //Set the text to the color CYAN
    deceased_patients++;
    puts("********************************");
    printf("*PARENT: Deceased Patients = %d *\n", deceased_patients);
    puts("********************************");
    usleep(500);

    if (deceased_patients >= 3)
    {
        printf("\033[01;33m"); //Set the text to the color CYAN
        printf("PARENT: MAXIMUM NUMBER OF PEOPLE DIEDPEOPLE DIED\n");
        printf("Exiting...");

        /*
        * Access, attach and reference the shared memory
        */
        struct MEMORY *memptr;
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

        for (int i = 0; i < 1000; i++)
        {
            if (memptr->patients[i] != -1)
                kill(memptr->patients[i], 9);
        }

        //killing the doctors
        for (int i = 0; i < doc_num; i++)
        {
            if (kill(doc_ids[i], 9) == -1)
                ;
        }
        printf("\033[0m"); //Resets the text to default color
        exit(0);
    }
    return;
}
