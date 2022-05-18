#include "SceneNode.h"
#include "SceneGraph.h"

namespace flower 
{
	SceneNode::SceneNode(const size_t id, const std::string& name)
        : m_id(id), m_name(name), m_transform(nullptr)
    {
        LOG_TRACE("SceneNode {0} with GUID {1} construct.", m_name.c_str(), m_id);
    }

    std::shared_ptr<SceneNode> SceneNode::create(const size_t id, const std::string& name, std::weak_ptr<Scene> scene)
    {
        auto res = std::make_shared<SceneNode>(id, name);

        res->m_transform = std::make_shared<Transform>();
        res->setComponent(res->m_transform);
        res->m_scene = scene;

        return res;
    }

    SceneNode::~SceneNode()
    {
        LOG_TRACE("SceneNode {0} with GUID {1} destroy.", m_name.c_str(), m_id);
    }

    std::shared_ptr<Component> SceneNode::getComponent(const char* id)
    {
        if(m_components.contains(id))
        {
            return m_components[id];
        }
        else
        {
            return nullptr;
        }
    }

    std::shared_ptr<Transform> SceneNode::getTransform()
    {
        return m_transform;
    }

    std::shared_ptr<SceneNode> SceneNode::getParent()
    {
        return m_parent.lock();
    }

    std::shared_ptr<SceneNode> SceneNode::getPtr()
    {
        return shared_from_this();
    }

    bool SceneNode::hasComponent(const char* id)
    {
        return m_components.count(id) > 0;
    }

    bool SceneNode::setName(const std::string& in)
    {
        auto scene = m_scene.lock();
        CHECK(scene && "scene can't be null if node still alive.");

        if (in != m_name)
        {
            m_name = in;
            scene->setDirty();
            return true;
        }

        return false;
    }

    // p is son of this node?
    bool SceneNode::isSon(std::shared_ptr<SceneNode> p)
    {
        std::shared_ptr<SceneNode> pp = p->m_parent.lock();
        while (pp)
        {
            if (pp->getId() == m_id)
            {
                return true;
            }
            pp = pp->m_parent.lock();
        }
        return false;
    }

    void SceneNode::updateDepth()
    {
        // maybe we should sync the depth on main tick to avoid 
        // repeat call when depth change more than once.
        if (auto parent = m_parent.lock())
        {
            m_depth = parent->m_depth + 1;
            for (auto& child : m_children)
            {
                child->updateDepth();
            }
        }
    }

    void SceneNode::addChild(std::shared_ptr<SceneNode> child)
    {
        m_children.push_back(child);
    }

    void SceneNode::setParent(std::shared_ptr<SceneNode> p)
    {
        // remove old parent's referece if exist.
        if (auto oldP = m_parent.lock())
        {
            oldP->removeChild(getId());
        }

        // prepare new reference.
        m_parent = p;
        p->addChild(shared_from_this());

        updateDepth();
        m_transform->invalidateWorldMatrix();
    }

    void SceneNode::removeChild(std::shared_ptr<SceneNode> o)
    {
        removeChild(o->getId());
    }

    void SceneNode::removeChild(size_t inId)
    {
        std::vector<std::shared_ptr<SceneNode>>::iterator it;

        size_t id = 0;
        while (m_children[id]->getId() != inId)
        {
            id++;
        }

        // swap to the end and pop.
        if (id < m_children.size())
        {
            std::swap(m_children[id], m_children[m_children.size() - 1]);
            m_children.pop_back();
        }
    }

    void SceneNode::selfDelete()
    {
        // when the parent's child ownership lost.
        // this node will reset automatically and all it's children reset recursive.
        if (auto parent = m_parent.lock())
        {
            parent->removeChild(getId());
        }
    }
}