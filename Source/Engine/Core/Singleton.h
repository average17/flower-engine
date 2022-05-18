#pragma once

namespace flower 
{
	template <typename T>
	class Singleton
	{
	private: 
		Singleton() { }
		Singleton(const Singleton& rhs) { }
		Singleton& operator= (const Singleton& rhs) { }

	public:
		static T* getInstance()
		{
			// after c++ 11 is thread safe.
			static T s { };
			return &s;
		}
	};
}