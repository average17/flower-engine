#pragma once
#include <memory>
#include <type_traits>
#include <vector>
#include <string>

#include "NonCopyable.h"
#include "TickData.h"

namespace flower {

	class ModuleManager;
	class Engine;

	class IRuntimeModule : private NonCopyable
	{
	protected:
		ModuleManager* m_moduleManager;
		std::string m_moduleName;

	public:
		IRuntimeModule(ModuleManager* in, std::string name) 
		: m_moduleManager(in), m_moduleName(name) 
		{
		
		}

		virtual ~IRuntimeModule() = default;

		virtual bool init() { return true; }
		virtual void tick(const TickData& tickData) = 0;
		virtual void release() {  }

		virtual void registerCheck() { }
	};

	template<typename T>
	constexpr void checkRuntimeModule() 
	{ 
		static_assert(std::is_base_of<IRuntimeModule, T>::value, 
			"This type doesn't match runtime module.");
	}

	class ModuleManager : private NonCopyable
	{
		friend Engine;
	private:
		Engine* m_engine;
		std::vector<std::unique_ptr<IRuntimeModule>> m_runtimeModules;
		
	public:
		~ModuleManager();

		Engine* getEngine() { return m_engine; }

		template<typename T>
		void registerRuntimeModule()
		{
			checkRuntimeModule<T>();

			auto newModule = std::make_unique<T>(this);
			newModule->registerCheck();

			m_runtimeModules.push_back(std::move(newModule));
		}

		bool init();
		void tick(const TickData& tickData);
		void release();

		template <typename T> 
		T* getRuntimeModule() const
		{
			checkRuntimeModule<T>();
			for (const auto& runtimeModule : m_runtimeModules)
			{
				if (runtimeModule && typeid(T) == typeid(*runtimeModule))
				{
					// we has used checkRuntimeModule function to ensure T is base of IRuntimeModule.
					// so just use static_cast.
					return static_cast<T*>(runtimeModule.get());
				}
			}
			return nullptr;
		}
	};

}