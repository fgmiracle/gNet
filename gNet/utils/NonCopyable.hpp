#ifndef __GNET_NONCOPY_H__
#define __GNET_NONCOPY_H__

namespace gNet
{
	// NonCopyable
	struct NonCopyable
	{
		NonCopyable & operator=(const NonCopyable&) = delete;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable() = default;
	};
}


#endif // !__NONCOPY_H__