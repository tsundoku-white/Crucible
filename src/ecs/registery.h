#pragma once

#include "src/core/pch.h"

#include "src/ecs/entity.h"
#include "src/ecs/component.h"

struct Registery
{
  Entity createEntity()
    {
      EntityID id;
      if (!m_avalibleEntityArray.empty())
      {
        id = m_avalibleEntityArray.back();
        m_avalibleEntityArray.pop_back();
        m_denceEntityArray[id] = id;
        return Entity(id);
      }
      else 
      {
        id = m_denceEntityArray.size();
        m_denceEntityArray.push_back(id);
        return Entity(id);
      }
    }
    void destroyEntity(Entity &entity)
    {
      if (!entity.isValid()) return;
      m_avalibleEntityArray.push_back(entity.getId());
      entity.invalidate();
    }
    template<typename T>
      T& add(EntityID id, T component)
      {
        return getPool<T>().insert(id, component);
      }
    template<typename T>
      void remove(EntityID id)
      {
        return getPool<T>().remove(id);
      }
    template<typename T>
      bool has(EntityID id)
      {
        return getPool<T>().has(id);
      }
    template<typename T>
      T& get(EntityID id)
      {
        return getPool<T>().get(id);
      }

    template<typename T>
      ComponentTypeID getComponentTypeId()
      {
        static ComponentTypeID id = nextCompTypeId();
        return id;
      }

    template<typename T>
      const std::vector<T> &getComponentsView() { return getPool<T>().getComponentView(); }
    const std::vector<EntityID> &getEntityView() const { return m_denceEntityArray; }
    private:
    template<typename T>
      Component_Pool<T>& getPool()
      {
        ComponentTypeID typeId = getComponentTypeId<T>();
        if (typeId >= m_pools.size())
          m_pools.resize(typeId + 1);
        if (!m_pools[typeId])
          m_pools[typeId] = std::make_unique<Component_Pool<T>>();
        return *static_cast<Component_Pool<T>*>(m_pools[typeId].get());
      }

    std::vector<std::unique_ptr<IPoolBase>> m_pools;
    std::vector<EntityID> m_avalibleEntityArray;
    std::vector<EntityID> m_denceEntityArray;
};

