#pragma once

#include "Core/RuntimeModule.h"

namespace flower {

	class Engine final : private NonCopyable
	{
	private:
		bool m_bInit = false;
		std::unique_ptr<ModuleManager> m_moduleManager{ nullptr };
		
	public:
		Engine() = default;
		~Engine();

		template <typename T>
		T* getRuntimeModule() const
		{
			return m_moduleManager->getRuntimeModule<T>();
		}

		template<typename T>
		void registerRuntimeModule()
		{
			m_moduleManager->registerRuntimeModule<T>();
		}

		bool beforeInit();
		bool init();
		bool tick(const TickData& tickData);
		void release();

		bool isEngineInit() const { return m_bInit; }
	};

	extern Engine GEngine;
}