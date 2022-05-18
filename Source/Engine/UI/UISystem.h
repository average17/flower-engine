#pragma once
#include "../Core/RuntimeModule.h"
#include <functional>
#include <unordered_map>

namespace flower
{
	class IconLibrary;

	// Please register uisystem before renderer.
	class UISystem : public IRuntimeModule
	{
	public:
		UISystem(ModuleManager* in);

		virtual bool init() override;
		virtual void tick(const TickData&) override;
		virtual void release() override;

		IconLibrary* getIconLibrary() { return m_iconLibrary.get(); }

		typedef void (RegisterImguiFunc)(const TickData&, size_t);

		void addImguiFunction(std::string name,std::function<RegisterImguiFunc>&& func);
		void removeImguiFunction(std::string name);

	private:
		uint32_t m_frameIndex = 0;
		std::unordered_map<std::string,std::function<RegisterImguiFunc>> m_uiFunctions{};
		std::unique_ptr<IconLibrary> m_iconLibrary;
	};
}