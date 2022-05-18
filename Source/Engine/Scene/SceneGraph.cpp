#include "SceneGraph.h"
#include "SceneManager.h"
#include "../Engine.h"
#include <queue>

namespace flower 
{
	size_t Scene::requireId()
	{
		CHECK(m_currentId < SIZE_MAX && "GUID max than size_t's max value.");

		m_currentId ++;
		return m_currentId;
	}

	SceneManager* Scene::getManager()
	{
		if(m_manager == nullptr)
		{
			m_manager = GEngine.getRuntimeModule<SceneManager>();
		}

		return m_manager;
	}

	std::shared_ptr<Scene> Scene::getptr()
	{
		return shared_from_this();
	}

	std::shared_ptr<Scene> Scene::create(std::string name)
	{
		auto newScene = std::shared_ptr<Scene>(new Scene());
		newScene->m_initName = name;
		return newScene;
	}

	Scene::~Scene()
	{
		m_root.reset();
	}

	bool Scene::init()
	{
		m_root = SceneNode::create(ROOT_ID, m_initName, shared_from_this());
		return true;
	}

	bool Scene::setDirty(bool bDirty)
	{
		if (m_bDirty != bDirty)
		{
			m_bDirty = bDirty;
			return true;
		}

		return false;
	}

	bool Scene::isDirty() const
	{
		return m_bDirty;
	}

	bool Scene::setName(const std::string& name)
	{
		return m_root->setName(name);
	}

	size_t Scene::getCurrentGUID() const
	{
		return m_currentId;
	}

	void Scene::tick(const TickData& tickData)
	{
		// update all transforms.
		loopNodeTopToDown([](std::shared_ptr<SceneNode> node)
		{
			node->m_transform->updateWorldTransform();

		},m_root);
	}

	const std::string& Scene::getName() const
	{
		return m_root->getName();
	}

	void Scene::deleteNode(std::shared_ptr<SceneNode> node)
	{
		node->selfDelete();
		setDirty();
	}

	std::shared_ptr<SceneNode> Scene::createNode(const std::string& name)
	{
		// use require id to avoid guid repeat problem.
		return SceneNode::create(requireId(), name, shared_from_this());
	}

	std::weak_ptr<Scene> SceneNode::getSceneReference() const
	{
		return m_scene;
	}

	void Scene::addChild(std::shared_ptr<SceneNode> child)
	{
		m_root->addChild(child);
	}

	void Scene::loopNodeDownToTop(
		const std::function<void(std::shared_ptr<SceneNode>)>& func, 
		std::shared_ptr<SceneNode> node)
	{
		auto& children = node->getChildren();
		for (auto& child : children)
		{
			loopNodeDownToTop(func, child);
		}

		func(node);
	}

	void Scene::loopNodeTopToDown(
		const std::function<void(std::shared_ptr<SceneNode>)>& func, 
		std::shared_ptr<SceneNode> node)
	{
		func(node);

		auto& children = node->getChildren();
		for (auto& child : children)
		{
			loopNodeTopToDown(func, child);
		}
	}

	std::shared_ptr<SceneNode> Scene::findNode(const std::string& name)
	{
		for (auto& root_node : m_root->getChildren())
		{
			std::queue<std::shared_ptr<SceneNode>> traverse_nodes{};
			traverse_nodes.push(root_node);

			while (!traverse_nodes.empty())
			{
				auto& node = traverse_nodes.front();
				traverse_nodes.pop();

				if (node->getName() == name)
				{
					return node;
				}

				for (auto& child_node : node->getChildren())
				{
					traverse_nodes.push(child_node);
				}
			}
		}

		if (name == m_root->getName())
		{
			return m_root;
		}

		return nullptr;
	}

	std::vector<std::shared_ptr<SceneNode>> Scene::findNodes(const std::string& name)
	{
		std::vector<std::shared_ptr<SceneNode>> results{ };

		for (auto& root_node : m_root->getChildren())
		{
			std::queue<std::shared_ptr<SceneNode>> traverse_nodes{};
			traverse_nodes.push(root_node);

			while (!traverse_nodes.empty())
			{
				auto& node = traverse_nodes.front();
				traverse_nodes.pop();

				if (node->getName() == name)
				{
					results.push_back(node);
				}

				for (auto& child_node : node->getChildren())
				{
					traverse_nodes.push(child_node);
				}
			}
		}

		if (name == m_root->getName())
		{
			results.push_back(m_root);
		}

		return results;
	}

	std::shared_ptr<SceneNode> Scene::getRootNode()
	{
		return m_root;
	}

	bool Scene::setParent(std::shared_ptr<SceneNode> parent, std::shared_ptr<SceneNode> son)
	{
		bool bNeedSet = false;

		auto oldP = son->getParent();

		// safe check.
		// son's orignal parent is nullptr / new parent is no the son of son.
		if (oldP == nullptr // always assign parent if old parent no exist.
			 // if new parent is no the son of the node, enable assign.
			|| (!son->isSon(parent) && parent->getId() != oldP->getId())
		){
			bNeedSet = true;
		}

		if (bNeedSet)
		{
			son->setParent(parent);
			setDirty();
			return true;
		}

		return false;
	}

	const std::vector<std::weak_ptr<Component>>& Scene::getComponents(const char* type_info)
	{
		return getManager()->m_components.at(type_info);
	}

	bool Scene::hasComponent(const char* type_info)
	{
		auto component = getManager()->m_components.find(type_info);
		return (component != getManager()->m_components.end() && !component->second.empty());
	}

	// sync scene node tree's transform form top to down to get current result.
	void Scene::flushSceneNodeTransform()
	{
		loopNodeTopToDown([](std::shared_ptr<SceneNode> node)
		{
			node->m_transform->updateWorldTransform();
		},m_root);
	}

	void Scene::setComponents(const char* type_info,std::vector<std::weak_ptr<Component>>&& components)
	{
		getManager()->m_components[type_info] = std::move(components);
		setDirty();
	}

}