#include "SceneManager.h"
#include "SceneGraph.h"
#include "SceneViewCamera.h"
#include "../Core/WindowData.h"
#include "../Engine.h"
#include "../Renderer/DeferredRenderer/DefferedRenderer.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/AtmosphereSky.h"

namespace flower
{
	SceneManager::SceneManager(ModuleManager* in)
		: IRuntimeModule(in, "SceneManager")
	{

	}

	bool SceneManager::init()
	{
		m_readyScenes.clear();
		m_sceneViewCamera = std::make_unique<SceneViewCamera>();

		m_sceneViewCamera->onInit(GEngine.getRuntimeModule<DefferedRenderer>());

		return true;
	}

	void SceneManager::tick(const TickData& tickData)
	{
		m_sceneViewCamera->onTick(tickData);

		for(auto& pair : m_readyScenes)
		{
			pair.second->tick(tickData);
		}
	}

	void SceneManager::release()
	{
		m_sceneViewCamera.reset();
	}

	bool SceneManager::unloadScene(std::string name)
	{
		if (m_readyScenes.contains(name))
		{
			m_readyScenes.erase(name);

			return true;
		}
		return false;
	}

	SceneViewCamera* SceneManager::getSceneViewCamera()
	{
		return m_sceneViewCamera.get();
	}

	std::shared_ptr<DirectionalLight> SceneManager::getDirectionalLightComponent()
	{
		if(auto cacheLight = m_cacheDirectionalLight.lock())
		{
			return cacheLight;
		}

		auto& lights = m_components[getTypeName<DirectionalLight>()];
		if(lights.size() > 0)
		{
			for (auto it = lights.begin(); it != lights.end(); it++)
			{
				if (auto compPtr = (*it).lock())
				{
					auto lightPtr = std::static_pointer_cast<DirectionalLight>(compPtr);
					CHECK(lightPtr);
					m_cacheDirectionalLight = lightPtr;
					return lightPtr;
				}
			}
		}
		return nullptr;
	}

	std::shared_ptr<AtmosphereSky> SceneManager::getAtmosphereSky()
	{
		if(auto cacheAtmosphereSky = m_cacheAtmosphereSky.lock())
		{
			return cacheAtmosphereSky;
		}

		auto& atmosphereSkys = m_components[getTypeName<AtmosphereSky>()];
		if(atmosphereSkys.size() > 0)
		{
			for (auto it = atmosphereSkys.begin(); it != atmosphereSkys.end(); it++)
			{
				if (auto compPtr = (*it).lock())
				{
					auto atmosphereSkyPtr = std::static_pointer_cast<AtmosphereSky>(compPtr);
					CHECK(atmosphereSkyPtr);

					m_cacheAtmosphereSky = atmosphereSkyPtr;
					return atmosphereSkyPtr;
				}
			}
		}
		return nullptr;
	}

	// note: static mesh should use precache and only collect when component change.
	void SceneManager::collectStaticMesh(std::vector<RenderMeshProxy>& inoutProxy)
	{
		inoutProxy = {};

		auto& staticMeshes = m_components[getTypeName<StaticMeshComponent>()];
		if(staticMeshes.size() > 0)
		{
			for (auto it = staticMeshes.begin(); it != staticMeshes.end(); it++)
			{
				if (auto compPtr = (*it).lock())
				{
					auto meshPtr = std::static_pointer_cast<StaticMeshComponent>(compPtr);
					CHECK(meshPtr);
					
					const auto& meshProxy = meshPtr->meshCollect();
					inoutProxy.push_back(meshProxy);
				}
			}
		}
	}

	std::vector<Scene*> SceneManager::getReadyScenes()
	{
		std::vector<Scene*> readyScenes{};

		for (auto& pair : m_readyScenes)
		{
			if (pair.second)
			{
				readyScenes.push_back(pair.second.get());
			}
		}

		// at least prepare one scene.
		if (readyScenes.size() == 0)
		{
			std::shared_ptr<Scene> newScene = createEmptyScene();

			CHECK(!m_readyScenes.contains(newScene->getName()) &&
				"New scene should not exist here.");
			
			readyScenes.push_back(newScene.get());

			// store to cache.
			m_readyScenes[newScene->getName()] = newScene;
		}

		return readyScenes;
	}

	std::shared_ptr<Scene> SceneManager::createEmptyScene()
	{
		auto newScene = Scene::create();

		newScene->init();
		newScene->setDirty(false);

		return newScene;
	}
}