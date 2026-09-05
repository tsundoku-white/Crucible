#pragma once

#include "src/core/pch.h"
#include "src/ecs/entity.h"

using ComponentTypeID = std::uint8_t;
static constexpr size_t INVALID_COMPONENT_INDEX = std::numeric_limits<size_t>::max();

inline ComponentTypeID nextCompTypeId()
{
    static ComponentTypeID counter = 0;
    return counter++;
}

struct IPoolBase
{
    virtual ~IPoolBase() = default;
    virtual void remove(EntityID id) = 0; 
};

template <typename T>
struct Component_Pool : IPoolBase
{
    T& get(EntityID id)
    {
        return m_denseComponent[m_sparse[id]];
    }

    bool has(EntityID id) const
    {
        return id < m_sparse.size() && m_sparse[id] != INVALID_COMPONENT_INDEX;
    }

    T& insert(EntityID id, T component)
    {
        if (id >= m_sparse.size())
            m_sparse.resize(id + 1, INVALID_COMPONENT_INDEX);

        size_t dense_index = m_denseComponent.size();
        m_denseComponent.push_back(std::move(component));
        m_denseToEntity.push_back(id);
        m_sparse[id] = dense_index;

        return m_denseComponent[dense_index];
    }

    void remove(EntityID id)
    {
        if (!has(id)) return;

        size_t index = m_sparse[id];
        size_t last  = m_denseComponent.size() - 1;

        m_denseComponent[index] = std::move(m_denseComponent[last]);
        m_denseToEntity[index] = m_denseToEntity[last];

        EntityID moved_entity = m_denseToEntity[index];
        m_sparse[moved_entity] = index;

        m_denseComponent.pop_back();
        m_denseToEntity.pop_back();

        m_sparse[id] = INVALID_COMPONENT_INDEX;
    }

    const std::vector<T>& view() const { return m_denseComponent; }
private:
    std::vector<size_t>   m_sparse;
    std::vector<T>        m_denseComponent;
    std::vector<EntityID> m_denseToEntity;
};
