#ifndef RWLOCKINGOBJ_H_
#define RWLOCKINGOBJ_H_
#include <pthread.h>

class RWLockingObject
{
public:
	RWLockingObject()
	{
		pthread_rwlock_init(&m_rwlock, 0);
	}

	~RWLockingObject()
	{
		pthread_rwlock_destroy(&m_rwlock);
	}

	void ReadLock() const
	{
		pthread_rwlock_rdlock(&m_rwlock);
	}

	void WriteLock()
	{
		pthread_rwlock_wrlock(&m_rwlock);
	}

	void Unlock() const
	{
		pthread_rwlock_unlock(&m_rwlock);
	}
private:
	mutable pthread_rwlock_t m_rwlock;
};

#endif
