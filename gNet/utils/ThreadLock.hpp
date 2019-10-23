#ifndef __GNET_THREADLOCK_H__
#define __GNET_THREADLOCK_H__

#include <atomic>

namespace gNet
{
	//spinlock
	class SpinLock
	{
	private:
		std::atomic_flag &m_Lock;

	public:
		SpinLock(std::atomic_flag &lock)
			:m_Lock(lock)
		{
			while (m_Lock.test_and_set()) {};
		}
		~SpinLock()
		{
			m_Lock.clear();
		}
	};
}

#endif // !__THREADLOCK_H__

