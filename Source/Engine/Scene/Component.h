#pragma once
#include <string>
#include <memory>
#include "../Core/Core.h"

namespace flower
{
	class SceneNode;

	#define COMPONENT_NAME_OVERRIDE(T) \
		inline const char* getComponentClassName() const { return getTypeName<T>(); } \
		static const char* getComponentStaticName()      { return getTypeName<T>(); } 

	class Component
	{
	protected:
		// reference of the owner node.
		std::weak_ptr<SceneNode> m_node;

	public:
		Component() = default;
		virtual ~Component();

		void setNode(std::weak_ptr<SceneNode> node) { m_node = node; }
		std::shared_ptr<SceneNode> getNode() { return m_node.lock(); }

		COMPONENT_NAME_OVERRIDE(Component);
	};

	template<typename T>
	inline void COMPONENT_TYPE_CHECK()
	{
		// static_assert(T::getComponentStaticName() != Component::getComponentStaticName());
		CHECK(T::getComponentStaticName() != Component::getComponentStaticName() && "Don't use componet base class.");
	}
}