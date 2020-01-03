#pragma once

#include <mutex>
#include <assert.h>

namespace GdiWindow
{

template<typename T>
struct Ref
{	
	Ref(std::mutex &m, T &t)
		: m(&m)
		, t(&t)
	{
		m.lock();
	}

	Ref(const Ref &o) = delete;

	Ref(Ref &&o)
		: m(o.m)
		, t(o.t)
	{
		o.m = nullptr;
		o.t = nullptr;
	}

	~Ref()
	{
		if (m)
			m->unlock();
	}

	T *operator->()
	{
		assert(t);
		assert(m);
		return t;
	}

	enum PreLocked { MutexPreLocked };
	Ref(PreLocked, std::mutex &m, T &t)
		: m(&m)
		, t(&t)
	{
	}

	std::mutex *m;
	T *t;
};

template<typename T>
struct OptionalRef
{
	OptionalRef(const OptionalRef &o) = delete;

	OptionalRef()
		: m(nullptr)
		, t(nullptr)
	{
	}

	OptionalRef(OptionalRef &&o)
		: m(o.m)
		, t(o.t)
	{
		o.m = nullptr;
		o.t = nullptr;
	}

	OptionalRef(Ref<T> &&o)
		: m(o.m)
		, t(o.t)
	{
		o.m = nullptr;
		o.t = nullptr;
	}

	~OptionalRef()
	{
		if (m)
			m->unlock();
	}

	T *operator->()
	{
		assert(t);
		assert(m);
		return t;
	}

	operator bool()
	{
		return !!t;
	}

	enum PreLocked { MutexPreLocked };
	OptionalRef(PreLocked, std::mutex &m, T &t)
		: m(&m)
		, t(&t)
	{
	}


	std::mutex *m;
	T *t;
};

template<typename T>
struct Guard
{
	operator Ref<T>()
	{
		return Ref<T>(m, t);
	}

	Ref<T> operator->()
	{
		return operator Ref<T>();
	}

	std::mutex m;
	T t;
};


template<typename T, typename U>
inline OptionalRef<T> stealMutexOptional(Ref<U> &o, T &t)
{
	OptionalRef<T> result(OptionalRef<T>::MutexPreLocked, *o.m, t);
	o.m = nullptr;
	o.t = nullptr;
	return std::move(result);
}

template<typename T, typename U>
inline Ref<T> stealMutex(Ref<U> &o, T &t)
{
	Ref<T> result(Ref<T>::MutexPreLocked, *o.m, t);
	o.m = nullptr;
	o.t = nullptr;
	return std::move(result);
}

template<typename T>
inline Ref<T> dereferenceAndEat(OptionalRef<T> &o)
{
	Ref<T> result(Ref<T>::MutexPreLocked, *o.m, *o.t);
	o.m = nullptr;
	o.t = nullptr;
	return std::move(result);
}


template<typename Ref>
inline void unlockMutex(Ref &ref)
{
	assert(ref.m);
	ref.m->unlock();
	ref.m = nullptr;
	ref.t = nullptr;
}

struct InverseMutexGuard
{
	InverseMutexGuard(const InverseMutexGuard&) = delete;

	InverseMutexGuard(std::mutex& mutex)
		: mutex(mutex)
	{
		mutex.unlock();
	}

	~InverseMutexGuard()
	{
		mutex.lock();
	}

	std::mutex &mutex;
};

}
