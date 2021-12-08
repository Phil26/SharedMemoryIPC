#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
using namespace std;

char shm_fn[] = "my_shm";
char sem_fn[] = "my_sem";

int main()
{
    caddr_t shmptr;
    unsigned int mode;
    int shmdes;
    sem_t* semdes;
    int SHM_SIZE;
    volatile int counter = 1;
    int write;

    mode = S_IRWXU | S_IRWXG;

    int pid = fork();
    if (pid == -1)
    {
        cout << "Error occured during creating the child process" << endl;
        return -1;
    }
    while (counter <= 1000)
    {
        srand(time(0));
        
        /* each process will flip the coin */
        write = rand() % 2 + 1;

        /*  if parent process ~process 1 */
        if (pid != 0)
        {
            cout << "Process 1 flip the coin." << endl;
            if (write == 1)
            {
                cout << "Process 1 obtained 1." << endl;
                cout << "Randomize again until 2 is obtained." << endl;
            }
            else if (write == 2)
            {
                /* Open the shared memory object */

                if ((shmdes = shm_open(shm_fn, O_CREAT | O_RDWR | O_TRUNC, mode)) == -1) 
                {
                    cout << "shm_open failure" << endl;
                    exit(1);
                }

                /* Preallocate a shared memory area */

                SHM_SIZE = sysconf(_SC_PAGE_SIZE);

                if (ftruncate(shmdes, SHM_SIZE) == -1) 
                {
                    cout << "ftruncate failure" << endl;
                    exit(2);
                }

                if ((shmptr = (caddr_t)mmap(0, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED,
                    shmdes, 0)) == (caddr_t)-1) 
                {
                    cout << "mmap failure" << endl;
                    exit(3);
                }

                /* Create a semaphore in locked state */

                semdes = sem_open(sem_fn, O_CREAT, 0644, 0);

                if (semdes == (void*)-1) {
                    cout << "sem_open failure" << endl;
                    exit(4);
                }

                /* Access to the shared memory area */

                cout << "Process 1 obtained 2." << endl;
                cout << "Process 1 write " << counter << " into the shared memory" << endl;
                
                /* increment the counter */
                counter++;

                /* Release the semaphore lock */

                sem_post(semdes);
                munmap(shmptr, SHM_SIZE);

                /* Close the shared memory object */

                close(shmdes);

                /* Close the Semaphore */

                sem_close(semdes);

                /* Delete the shared memory object */

                shm_unlink(shm_fn);
            }
        }

        /* if child process ~ process 2 */
        else if (pid == 0)
        {
            cout << "Process 2 flip the coin." << endl;
            if (write == 1)
            {
                cout << "Process 2 obtained 1." << endl;
                cout << "Randomize again until 2 is obtained." << endl;
            }
            else if (write == 2)
            {
                /* Open the shared memory object */

                SHM_SIZE = sysconf(_SC_PAGE_SIZE);

                if ((shmdes = shm_open(shm_fn, O_RDWR, 0)) == -1) 
                {
                    cout << "shm_open failure" << endl;
                    exit(5);
                }

                if ((shmptr = (caddr_t)mmap(0, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED,
                    shmdes, 0)) == (caddr_t)-1) 
                {
                    cout << "mmap failure" << endl;
                    exit(6);
                }

                /* Open the Semaphore */

                semdes = sem_open(sem_fn, 0, 0644, 0);

                if (semdes == (void*)-1) 
                {
                    cout << "sem_open failure" << endl;
                    exit(7);
                }

                /* Lock the semaphore */

                if (!sem_wait(semdes)) 
                {

                    /* Access to the shared memory area */

                    cout << "Process 2 obtained 2." << endl;
                    cout << "Process 2 write " << counter << " into the shared memory" << endl;

                    // increment the counter
                    counter++;

                    /* Release the semaphore lock */

                    sem_post(semdes);

                }

                munmap(shmptr, SHM_SIZE);

                /* Close the shared memory object */

                close(shmdes);

                /* Close the Semaphore */

                sem_close(semdes);
                sem_unlink(sem_fn);

            }
        }
    }
    return 0;
}

