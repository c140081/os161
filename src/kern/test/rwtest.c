/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

/*
 * Use these stubs to test your reader-writer locks.
 */

#define CREATELOOPS	8
#define NLOCKLOOPS    20
#define NTHREADS      32

static volatile unsigned long testval1;

static struct semaphore *donesem = NULL;
static struct rwlock *testrw = NULL;

struct spinlock status_lock;
static bool test_status = TEST161_FAIL;





static
bool
failif(bool condition) {
	if (condition) {
		spinlock_acquire(&status_lock);
		test_status = TEST161_FAIL;
		spinlock_release(&status_lock);
	}
	return condition;
}



static
void
rwtestreader(void *junk, unsigned long num)
{
	(void)junk;

	int i;
	unsigned int tempval;

	for (i=0; i<NLOCKLOOPS; i++) {
		kprintf_t(".");
		rwlock_acquire_read(testrw);
    		tempval = testval1 ;
    		kprintf_n("for reader %lu, tempval1 = %u\n", num, tempval);

		random_yielder(4);

    		kprintf_n("after 1 random_yielder, for reader %lu, testval1 = %lu\n", num, testval1);

		if (testval1 != tempval) {
			failif(true);
		}

		rwlock_release_read(testrw);
	}


	V(donesem);
	return;
}


static
void
rwtestwriter(void *junk, unsigned long num)
{
	(void)junk;

	int i;

	for (i=0; i<NLOCKLOOPS; i++) {
		kprintf_t(".");
		rwlock_acquire_write(testrw);
		testval1 = num ;
    		kprintf_n("for writer %lu, new testval1 = %lu\n", num, testval1);
    
		random_yielder(4);

   		kprintf_n("after 1 random_yielder, for writer %lu, testval1 = %lu\n", num, testval1);

		if (testval1 != num) {
			failif(true);
		}

		rwlock_release_write(testrw);
	}

	V(donesem);
	return;
}




int
rwtest(int nargs, char **args)
{
	(void)nargs;
	(void)args;

  	testval1 = NTHREADS-1 ;

	int i, result;

	kprintf_n("Starting rwtest 1...\n");
	for (i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");

		testrw = rwlock_create("testrw");
		if (testrw == NULL) {
			panic("rwtest 1: rwlock_create failed\n");
		}

    		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwtest1: sem_create failed\n");
		}

		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrw);
			sem_destroy(donesem);
		}
	}

	spinlock_init(&status_lock);
	test_status = TEST161_SUCCESS;

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
    		if( (i%5) != 0 ){
      			result = thread_fork("rwtest", NULL, rwtestreader, NULL, i);
    		} else {    
      			result = thread_fork("rwtest", NULL, rwtestwriter, NULL, i);
    		}

		if (result) {
			panic("rwtest1: thread_fork failed: %s\n",
			strerror(result));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		P(donesem);                 // V(donesem) in rwtestthread
	}

  	rwlock_destroy(testrw);
	sem_destroy(donesem);
	testrw = NULL;
  	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwtest1");

	return 0;
}








int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	(void)nargs;
	(void)args;

  	testval1 = NTHREADS-1 ;

	int i, result;

	kprintf_n("Starting rwtest 2...\n");
	for (i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");

		testrw = rwlock_create("testrw");
		if (testrw == NULL) {
			panic("rwtest 2: rwlock_create failed\n");
		}

    		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwtest2: sem_create failed\n");
		}

		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrw);
			sem_destroy(donesem);
		}
	}

	spinlock_init(&status_lock);
	test_status = TEST161_FAIL;

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
      		result = thread_fork("rwtest2", NULL, rwtestreader, NULL, i);
		if ( (!testrw->writer_count) && testrw->rw_sem->sem_count == 0 ){
			kprintf_t("rw_sem->sem_count = %u\n", testrw->rw_sem->sem_count);
			test_status = TEST161_SUCCESS;

		}

		if (result) {
			panic("rwtest2: thread_fork failed: %s\n",
			strerror(result));
		}

	}

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		P(donesem);                 // V(donesem) in rwtestreader
	}

  	rwlock_destroy(testrw);
	sem_destroy(donesem);
	testrw = NULL;
  	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwt2");

	return 0;
}



int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;



	kprintf_n("Starting rwtest 3...\n");
	for (int i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");

		testrw = rwlock_create("testrw");
		if (testrw == NULL) {
			panic("rwtest 3: rwlock_create failed\n");
		}

    		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwtest3: sem_create failed\n");
		}

		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrw);
			sem_destroy(donesem);
		}
	}


	secprintf(SECRET, "Should panic...", "rwt3");
	rwlock_release_write(testrw);

	/* Should not get here on success. */

  	rwlock_destroy(testrw);
	sem_destroy(donesem);
	testrw = NULL;
  	donesem = NULL;


	kprintf_n("\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}



int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("Starting rwtest 4...\n");
	for (int i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");

		testrw = rwlock_create("testrw");
		if (testrw == NULL) {
			panic("rwtest 4: rwlock_create failed\n");
		}

    		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwtest4: sem_create failed\n");
		}

		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrw);
			sem_destroy(donesem);
		}
	}


	secprintf(SECRET, "Should panic...", "rwt4");
	rwlock_release_read(testrw);

	/* Should not get here on success. */

  	rwlock_destroy(testrw);
	sem_destroy(donesem);
	testrw = NULL;
  	donesem = NULL;

	kprintf_n("\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("Starting rwtest 5...\n");
	for (int i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");

		testrw = rwlock_create("testrw");
		if (testrw == NULL) {
			panic("rwtest 5: rwlock_create failed\n");
		}

    		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwtest5: sem_create failed\n");
		}

		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrw);
			sem_destroy(donesem);
		}
	}


	secprintf(SECRET, "Should panic...", "rwt5");
//	rwlock_release_read(testrw);

	/* Should not get here on success. */

  	rwlock_destroy(testrw);
	sem_destroy(donesem);
	testrw = NULL;
  	donesem = NULL;

	kprintf_n("\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}
