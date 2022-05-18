#pragma once

#include "Component/transform.h"

namespace flower
{
    class Scene;

    class SceneNode : public std::enable_shared_from_this<SceneNode>
    {
        friend Scene;

    private:
        size_t      m_id; // id of scene node.
        size_t      m_depth = 0; // the scene node depth of the scene tree.

        std::string m_name  = "SceneNode"; // inspector name.

        // reference of parent.
        std::weak_ptr<SceneNode> m_parent;

        // reference of scene.
        std::weak_ptr<Scene> m_scene;

        // owner of transform.
        std::shared_ptr<Transform> m_transform;

        // owner of components without transform type.
        std::unordered_map<const char*, std::shared_ptr<Component>> m_components;

        // owner of children.
        std::vector<std::shared_ptr<SceneNode>> m_children;

    public:
        // don't call this gay. this one just use for cereal, use SceneNode::create.
        SceneNode() = default;

        // don't call this gay. this one just use for cereal, use SceneNode::create.
        SceneNode(const size_t id, const std::string & name);

        static std::shared_ptr<SceneNode> create(const size_t id, const std::string& name, std::weak_ptr<Scene> scene);

        virtual ~SceneNode();

        const auto& getId()       const { return m_id; }
        const auto& getName()     const { return m_name; }
        const auto& getChildren() const { return m_children; }
        const auto& getDepth()    const { return m_depth; }
        const bool  isRoot()      const { return m_depth == 0; }

        std::shared_ptr<Component> getComponent(const char* id);

        template <typename T>
        std::shared_ptr<T> getComponent()
        {
            // disable call getComponent<Component>().
            // this action is no sense.
            COMPONENT_TYPE_CHECK<T>();
            return std::dynamic_pointer_cast<T>(getComponent(getTypeName<T>()));
        }

        std::shared_ptr<Transform> getTransform();

        std::shared_ptr<SceneNode> getParent();
       
        std::shared_ptr<SceneNode> getPtr();

        std::weak_ptr<Scene> getSceneReference() const;

        bool hasComponent(const char* id);

        template <class T> bool hasComponent() 
        { 
            COMPONENT_TYPE_CHECK<T>();
            return hasComponent(getTypeName<T>());
        }

        bool setName(const std::string& in);

        // p is the son of this node.
        bool isSon(std::shared_ptr<SceneNode> p);

    private:
        void updateDepth();

        void addChild(std::shared_ptr<SceneNode> child);

        // set parent.
        void setParent(std::shared_ptr<SceneNode> p);

        void removeChild(std::shared_ptr<SceneNode> o);
        void removeChild(size_t id);

        template<typename T>
        void setComponent(std::shared_ptr<T> component)
        {
            COMPONENT_TYPE_CHECK<T>();

            component->setNode(getPtr());
            auto it = m_components.find(component->getComponentClassName());
            if (it != m_components.end())
            {
                it->second = component;
            }
            else
            {
                m_components.insert(std::make_pair(component->getComponentClassName(), component));
            }
        }

        void selfDelete();

        // remove component.
        template<class T>
        void removeComponent()
        {
            // all component will override getComponentStaticName, so this is also a check.
            COMPONENT_TYPE_CHECK<T>();
            m_components.erase(T::getComponentStaticName());
        }
    };
}