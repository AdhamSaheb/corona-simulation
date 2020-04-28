#ifndef __LOCAL_H__
#define __LOCAL_H__

/*
 * Common header file for Message Queue Example
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>    


/* This declaration is *MISSING* is many solaris environments.
   It should be in the <sys/sem.h> file but often is not! If 
   you receive a duplicate definition error message for semun
   then comment out the union declaration.
   

union semun {
  int              val;
  struct semid_ds *buf;
  ushort          *array; 
};
*/

#define SHM_SIZE  1024

// for semaphore operations 
struct sembuf acquire = {0, -1, SEM_UNDO}, 
              release = {0,  1, SEM_UNDO};


typedef struct MEMORY
{
    int doctors [10000];
    int patients [20000];

} MEMORY;

struct mesg_buffer
{
    long mesg_type;
    char mesg_text[100];
    int sender_pid;
} message;

enum {DOCTOR,PATIENT};

// PATIENT SYMPTOMPS 
typedef struct INFO
{
    int AGE;
    int COVID19_FEVER;
    int COVID19_COUGH;
    int COVID19_BREATH;
    int BLOOD_HYPER;
    int HEART_RESP;
    int CANCER;
} INFO;

#endif
