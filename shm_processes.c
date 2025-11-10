#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>

#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

static int ShmID = -1;
static int *BankAccount = NULL;
static sem_t *mutex = NULL;
static pid_t child_pid = -1;



void  ClientProcess(int []);

static inline int randi(int max_inclusive) {
     if (max_inclusive <= 0) return 0;
     return rand() % (max_inclusive + 1);
}
static inline void lock_sem(sem_t *m){ while (sem_wait(m) == -1 && errno == EINTR) {} }
static inline void unlock_sem(sem_t *m){ sem_post(m); }

int  main(int  argc, char *argv[])
{
     int    ShmID_local;
     int    *ShmPTR;
     pid_t  pid;
     int    status;

     const char *SEM_NAME = "psdd_sem";
     int nloop = 10;

     srand((unsigned)time(NULL) ^ (unsigned)getpid());


     if (argc != 5) {
          printf("Use: %s #1 #2 #3 #4\n", argv[0]);
          exit(1);
     }

     ShmID = shmget(IPC_PRIVATE, 4*sizeof(int), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          exit(1);
     }
     printf("Server has received a shared memory of four integers...\n");

     ShmPTR = (int *) shmat(ShmID, NULL, 0);
     if ((long)ShmPTR == -1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
     }
     printf("Server has attached the shared memory...\n");

     ShmPTR[0] = atoi(argv[1]);
     ShmPTR[1] = atoi(argv[2]);
     ShmPTR[2] = atoi(argv[3]);
     ShmPTR[3] = atoi(argv[4]);
     printf("Server has filled %d %d %d %d in shared memory...\n",
            ShmPTR[0], ShmPTR[1], ShmPTR[2], ShmPTR[3]);

     sem_unlink(SEM_NAME);
     mutex = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1); 
     if (mutex == SEM_FAILED) {
          perror("sem_open");
          shmdt((void *) ShmPTR);
          shmctl(ShmID, IPC_RMID, NULL);
          exit(1);
     }

     printf("Server is about to fork a child process...\n");
     pid = fork();
     if (pid < 0) {
          printf("*** fork error (server) ***\n");
          sem_close(mutex);
          sem_unlink(SEM_NAME);
          shmdt((void *) ShmPTR);
          shmctl(ShmID, IPC_RMID, NULL);
          exit(1);
     }
     else if (pid == 0) {
          ClientProcess(ShmPTR);
          shmdt((void *) ShmPTR);
          sem_close(mutex);
          exit(0);
     }

     for (int i = 0; i < nloop; i++) {
          sleep(randi(5));
          printf("Dear Old Dad: Attempting to Check Balance\n");
          int localBalance;
          lock_sem(mutex);
          localBalance = ShmPTR[0];
          unlock_sem(mutex);
          if ((randi(1000) & 1) == 0) {
               if (localBalance < 100) {
                    int amount = randi(100);
                    lock_sem(mutex);
                    localBalance = ShmPTR[0];
                    localBalance += amount;
                    ShmPTR[0] = localBalance;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                    unlock_sem(mutex);
               } else {
                    printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
               }
          } else {
               printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
          }
          fflush(stdout);
     }

     wait(&status);

     shmdt((void *) ShmPTR);
     shmctl(ShmID, IPC_RMID, NULL);
     sem_close(mutex);
     sem_unlink(SEM_NAME);

     exit(0);
}


void  ClientProcess(int  SharedMem[])
{
     int nloop = 10;
     printf("   Client process started\n");
     for (int i = 0; i < nloop; i++) {
          sleep(randi(5));
          printf("Poor Student: Attempting to Check Balance\n");
          int localBalance;
          lock_sem(mutex);
          localBalance = SharedMem[0];
          unlock_sem(mutex);
          if ((randi(1000) & 1) == 0) {
               int need = randi(50);
               printf("Poor Student needs $%d\n", need);
               lock_sem(mutex);
               localBalance = SharedMem[0];
               if (need <= localBalance) {
                    localBalance -= need;
                    SharedMem[0] = localBalance;
                    printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
               } else {
                    printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
               }
               unlock_sem(mutex);
          } else {
               printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
          }
          fflush(stdout);
     }
     printf("   Client is about to exit\n");
}
