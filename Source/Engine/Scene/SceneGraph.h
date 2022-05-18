#pragma once

#include "../Core/Core.h"
#include "../Core/RuntimeModule.h"
#include "Component.h"
#include "SceneNode.h"

namespace flower
{
	class SceneManager;

	class Scene : public std::enable_shared_from_this<Scene>
	{
	private:
		SceneManager* m_manager = nullptr;

		// just use for temporal store init name.
		std::string m_initName = "untitled";

		// root node id.
		const size_t ROOT_ID = 0;

		// cache scene node index. use for guid.
		size_t m_currentId = ROOT_ID;

		// owner of the root node.
		std::shared_ptr<SceneNode> m_root;

		// is scene dirty?
		bool m_bDirty = false;

	private:
		// require guid of scene node in this scene.
		size_t requireId();
		Scene() = default;

		SceneManager* getManager();

	public:
		virtual ~Scene();

		std::shared_ptr<Scene> getptr();
		static std::shared_ptr<Scene> create(std::string name = "untitled");

		bool init();

		bool setDirty(bool bDirty = true);
		bool isDirty() const;

		bool setName(const std::string& name);

		size_t getCurrentGUID() const;

		void tick(const TickData& tickData);

		const std::string& getName() const;

		void deleteNode(std::shared_ptr<SceneNode> node);
		std::shared_ptr<SceneNode> createNode(const std::string& name);

		// add child for root.
		void addChild(std::shared_ptr<SceneNode> child);

		// post-order loop.
		void loopNodeDownToTop(const std::function<void(std::shared_ptr<SceneNode>)>& func, std::shared_ptr<SceneNode> node);
		
		// pre-order loop.
		void loopNodeTopToDown(const std::function<void(std::shared_ptr<SceneNode>)>& func, std::shared_ptr<SceneNode> node);

		// loop the whole graph to find first same name scene node, this is a slow function.
		std::shared_ptr<SceneNode> findNode(const std::string& name);

		// find all same name nodes, this is a slow function.
		std::vector<std::shared_ptr<SceneNode>> findNodes(const std::string& name);

		std::shared_ptr<SceneNode> getRootNode();

		bool setParent(std::shared_ptr<SceneNode> parent, std::shared_ptr<SceneNode> son);

		const std::vector<std::weak_ptr<Component>>& getComponents(const char* type_info);

		bool hasComponent(const char* type_info);

		// update whole graph's transform.
		void flushSceneNodeTransform();

		template<typename T>
		void addComponent(std::shared_ptr<T> component, std::shared_ptr<SceneNode> node);

		template <typename T>
		bool hasComponent() const
		{
			COMPONENT_TYPE_CHECK<T>();
			return hasComponent(T::getComponentStaticName());
		}

		template<typename T>
		bool removeComponent(std::shared_ptr<SceneNode> node)
		{
			COMPONENT_TYPE_CHECK<T>();
			if (node->hasComponent<T>())
			{
				setDirty();
				node->removeComponent<T>();

				return true;
			}

			return false;
		}

		void setComponents(const char* type_info, std::vector<std::weak_ptr<Component>>&& components);
		

		template <class T>
		void setComponents(std::vector<std::weak_ptr<T>>&& components)
		{
			COMPONENT_TYPE_CHECK<T>();
			setComponents(T::getComponentStaticName(), std::move(components));
			setDirty();
		}

		template <class T>
		void clearComponents()
		{
			COMPONENT_TYPE_CHECK<T>();
			setComponents(T::getComponentStaticName(), {});
			setDirty();
		}

		template <class T>
		std::vector<std::weak_ptr<T>> getComponents() const
		{
			COMPONENT_TYPE_CHECK<T>();

			std::vector<std::weak_ptr<T>> result;
			auto id = T::getComponentStaticName();
			if (hasComponent(id))
			{
				const auto& scene_components = getComponents(id);

				result.resize(scene_components.size());
				std::transform(scene_components.begin(), scene_components.end(), result.begin(),
					[](const std::weak_ptr<Component>& component) -> std::weak_ptr<T>
					{
						return std::static_pointer_cast<T>(component.lock());
					});
			}
			return result;
		}
	};
}