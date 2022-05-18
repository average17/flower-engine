#pragma once
#include "../Core/RuntimeModule.h"
#include "Component/StaticMeshComponent.h"
#include "SceneGraph.h"
#include <map>

namespace flower 
{
	class SceneViewCamera;
	class OffscreenRenderer;
	class DirectionalLight;
	class AtmosphereSky;

	class SceneManager : public IRuntimeModule
	{
		// cache global scene components, for loop convenience.
		using ComponentContainer = 
			std::unordered_map<const char*, std::vector<std::weak_ptr<Component>>>;

		friend Scene;
	public:
		SceneManager(ModuleManager* in);

		virtual bool init() override;
		virtual void tick(const TickData&) override;
		virtual void release() override;

		std::vector<Scene*> getReadyScenes();
		bool unloadScene(std::string name);
		SceneViewCamera* getSceneViewCamera();

		// get first directionalLightComponent.
		std::shared_ptr<DirectionalLight> getDirectionalLightComponent();

		// get first getAtmosphereSky.
		std::shared_ptr<AtmosphereSky> getAtmosphereSky();

		void collectStaticMesh(std::vector<RenderMeshProxy>& inoutProxy);
	private:
		std::shared_ptr<Scene> createEmptyScene();

		// cache whole world's components.
		ComponentContainer m_components = {};

	private:
		std::weak_ptr<DirectionalLight> m_cacheDirectionalLight;
		std::weak_ptr<AtmosphereSky> m_cacheAtmosphereSky;

		// all scenes load in world.
		std::map<std::string, std::shared_ptr<Scene>> m_readyScenes { };

		std::unique_ptr<SceneViewCamera> m_sceneViewCamera;
	};

	template<typename T>
	void Scene::addComponent(std::shared_ptr<T> component, std::shared_ptr<SceneNode> node)
	{
		COMPONENT_TYPE_CHECK<T>();
		if (component && !node->hasComponent(T::getComponentStaticName()))
		{
			node->setComponent(component);
			getManager()->m_components[T::getComponentStaticName()].push_back(component);
		}
	}
}