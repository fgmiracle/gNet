#ifndef __GNET_SINGLETON_H__
#define __GNET_SINGLETON_H__

//#include <memory>

namespace gNet {
	template <typename T>
	class Singleton {
	public:
		template<typename ...Args>
		static T& getInstance(Args&&... args)
		{
			static std::once_flag s_flag;
			std::call_once(s_flag, [&]() {
				//instance = std::make_unique<T>(std::forward<Args>(args)...);
				instance.reset(new T(std::forward<Args>(args)...));
			});

			return *instance;
		}

		~Singleton() = default;
	private:
		Singleton() = default;
		
		Singleton(const Singleton&) = delete;
		
		Singleton& operator=(const Singleton&) = delete;
	private:
		static std::unique_ptr<T> instance;
	};

	template<typename T>
	std::unique_ptr<T> Singleton<T>::instance = nullptr;
}

#endif
