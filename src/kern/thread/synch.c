/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;

	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);
	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	// add stuff here as needed

	lock->lk_wchan = wchan_create(lock->lk_name);
	if (lock->lk_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

	spinlock_init(&lock->lk_lock);
	lock->lk_locked = false;
	lock->lk_holderthread = NULL ;

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);

	// add stuff here as needed
        KASSERT( !lock->lk_locked );

              
        spinlock_cleanup(&lock->lk_lock);
	wchan_destroy(lock->lk_wchan);
	kfree(lock->lk_name);
	lock->lk_locked = NULL;
	lock->lk_holderthread = NULL ;

	kfree(lock); 
}

void
lock_acquire(struct lock *lock)
{
	// Write this
  	KASSERT(lock != NULL);
	KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&lock->lk_lock);

	while ( lock->lk_locked ) {

			wchan_sleep(lock->lk_wchan, &lock->lk_lock);

	}

	KASSERT(lock->lk_locked != true);
	lock->lk_locked = true ;
	lock->lk_holderthread = curthread ;

	spinlock_release(&lock->lk_lock);

// 	(void)lock;  // suppress warning until code gets written
}

void
lock_release(struct lock *lock)
{
	// Write this
	KASSERT(lock != NULL);
	KASSERT( lock->lk_locked );

	spinlock_acquire(&lock->lk_lock);

	if( lock_do_i_hold(lock) ) {
			lock->lk_locked = false;
                        lock->lk_holderthread = NULL;
			wchan_wakeone(lock->lk_wchan, &lock->lk_lock);
	}

	spinlock_release(&lock->lk_lock);

// 	(void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
	// Write this
	if(lock->lk_holderthread == curthread )
			return true;

	return false;

//	(void)lock;  // suppress warning until code gets written

//	return true; // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}

	// add stuff here as needed

	cv->cv_wchan = wchan_create(cv->cv_name);
	if (cv->cv_wchan == NULL) {
		kfree(cv->cv_name);
		kfree(cv);
		return NULL;
	}

	spinlock_init(&cv->cv_lock);  // comment / decomment 

	return cv;
}


void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);

	// add stuff here as needed
	wchan_destroy(cv->cv_wchan);
	spinlock_cleanup(&cv->cv_lock);  // comment / decomment 
	kfree(cv->cv_name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT( cv != NULL );
	KASSERT( lock != NULL );
	KASSERT( lock->lk_locked );
	KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&cv->cv_lock);  // comment / decomment 

	if ( lock_do_i_hold(lock) ){

                     lock_release(lock);
		     wchan_sleep(cv->cv_wchan, &cv->cv_lock);  // comment / decomment 	
//                     wchan_sleep(cv->cv_wchan, &lock->lk_lock);
                     spinlock_release(&cv->cv_lock);
                     lock_acquire(lock);
	    	     spinlock_acquire(&cv->cv_lock); 

	}
	spinlock_release(&cv->cv_lock);


//	(void)cv;    // suppress warning until code gets written
//	(void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT( cv != NULL );
	KASSERT( lock != NULL );

	spinlock_acquire(&cv->cv_lock);  // comment / decomment 	

	if ( lock_do_i_hold(lock) ){

				 wchan_wakeone(cv->cv_wchan, &cv->cv_lock);  // comment / decomment 	
//				 wchan_wakeone(cv->cv_wchan, &lock->lk_lock);

	}
	spinlock_release(&cv->cv_lock);  // comment / decomment 	

//	(void)cv;    // suppress warning until code gets written
//	(void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT( cv != NULL );
	KASSERT( lock != NULL );

	spinlock_acquire(&cv->cv_lock);
//	KASSERT( !wchan_isempty(cv->cv_wchan, &cv->cv_lock) );

	if ( lock_do_i_hold(lock) ){

				 wchan_wakeall(cv->cv_wchan, &cv->cv_lock);
//				 wchan_wakeall(cv->cv_wchan, &lock->lk_lock);
	}

	spinlock_release(&cv->cv_lock);  // comment / decomment 	

//	(void)cv;    // suppress warning until code gets written
//	(void)lock;  // suppress warning until code gets written
}






///////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// read-write lock


struct rwlock * rwlock_create(const char * name )
{
	struct rwlock *rw;

	rw = kmalloc(sizeof(*rw));
	if (rw == NULL) {
		return NULL;
	}

	rw->rw_name = kstrdup(name);
	if (rw->rw_name==NULL) {
		kfree(rw);
		return NULL;
	}

	rw->rw_wchan = wchan_create(rw->rw_name);
	if (rw->rw_wchan == NULL) {
		kfree(rw->rw_name);
		kfree(rw);
		return NULL;
	}

	rw->rw_lock= lock_create(rw->rw_name);
	if (rw->rw_lock == NULL) {
		wchan_destroy(rw->rw_wchan);
		kfree(rw->rw_name);
		kfree(rw);
		return NULL;
	}

	rw->max_readers = 5;

	rw->rw_sem = sem_create(rw->rw_name, rw->max_readers);
	if (rw->rw_sem == NULL) {
		wchan_destroy(rw->rw_wchan);
		lock_destroy(rw->rw_lock);
		kfree(rw->rw_name);
		kfree(rw);
		return NULL;
	}

	rw->writer_count= false;
  	rw->writer_aging= 0;
	spinlock_init(&rw->rw_spinlock);  

	return rw;
}



void rwlock_destroy(struct rwlock * rw){

	KASSERT(rw != NULL);

	wchan_destroy(rw->rw_wchan);
	spinlock_cleanup(&rw->rw_spinlock);
	lock_destroy(rw->rw_lock);
	sem_destroy(rw->rw_sem);
	kfree(rw->rw_name);
	kfree(rw);

}



void rwlock_acquire_read(struct rwlock * rw){

	lock_acquire(rw->rw_lock);
	while( (rw->rw_sem->sem_count <= 0) || rw->writer_aging >= rw->max_readers ){   // prevent starvation
 
		lock_release(rw->rw_lock);
		spinlock_acquire(&rw->rw_spinlock);  // protect wchan_sleep or wakeall
		wchan_sleep(rw->rw_wchan, &rw->rw_spinlock);
		spinlock_release(&rw->rw_spinlock); 
		lock_acquire(rw->rw_lock);
	}
 	P(rw->rw_sem);

	if ( rw->writer_count ) {
  	rw->writer_aging++ ;
	}

	lock_release(rw->rw_lock);
}


void rwlock_release_read(struct rwlock * rw){

	KASSERT( rw->rw_sem->sem_count < rw->max_readers );

	lock_acquire(rw->rw_lock);
	V(rw->rw_sem);


//	if( !wchan_isempty(rw->rw_wchan, &rw->rw_spinlock) ){

		spinlock_acquire(&rw->rw_spinlock);
		wchan_wakeall(rw->rw_wchan, &rw->rw_spinlock);
		spinlock_release(&rw->rw_spinlock);
//	}
	lock_release(rw->rw_lock);

}

void rwlock_acquire_write(struct rwlock * rw){

	lock_acquire(rw->rw_lock);
	rw->writer_count = true; 

 	while ( rw->rw_sem->sem_count < rw->max_readers ) {

		lock_release(rw->rw_lock);
		spinlock_acquire(&rw->rw_spinlock);
		wchan_sleep(rw->rw_wchan, &rw->rw_spinlock);
		spinlock_release(&rw->rw_spinlock);
		lock_acquire(rw->rw_lock);     		
	}

	for( unsigned int i = 0 ; i < rw->max_readers ; i++) {
		P(rw->rw_sem);
	}
	rw->writer_aging = 0;
       
	lock_release(rw->rw_lock);

}


void rwlock_release_write(struct rwlock * rw){

	lock_acquire(rw->rw_lock);
	KASSERT( rw->rw_sem->sem_count == 0);
	rw->writer_count = false; 

	for(unsigned int  i = 0 ; i < rw->max_readers ; i++)
	{
		V(rw->rw_sem);
	}

//	if( !wchan_isempty(rw->rw_wchan, &rw->rw_spinlock) ){

		spinlock_acquire(&rw->rw_spinlock);
		wchan_wakeall(rw->rw_wchan, &rw->rw_spinlock);
		spinlock_release(&rw->rw_spinlock);
//	}

	lock_release(rw->rw_lock);


}
