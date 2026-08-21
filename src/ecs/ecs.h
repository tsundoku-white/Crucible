#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <print>
#include <strings.h>
#include <utility>
#include <vector>

using EntityID = std::uint64_t; 

constexpr EntityID NULL_ENTITY_ID = std::numeric_limits<EntityID>::max(); 

struct Entity 
{
  explicit Entity(EntityID id) : m_id(id) {}

  EntityID get_id() const { return m_id; }
  bool is_null()    const { return m_id == NULL_ENTITY_ID; }
  bool is_static()  const { return m_static; }

  private:
  EntityID m_id;
  bool m_static = false;
};

template <typename T>
class IPool 
{
  public:
    T& insert_component(EntityID id, T component)
    {
      if (has_component(id))
      {
        return get_component(id);
      }
      if (id >= m_sparse.size())
        m_sparse.resize(id + 1, INVALID_INDEX);

      m_sparse[id] = m_sparse.size();
      m_dense.push_back(std::move(component));
      m_entity_dense.push_back(id);
      return m_dense.back();
    }

    T& get_component(EntityID &id)
    {
      return m_dense[m_sparse[id]];
    }

    bool has_component(EntityID &id)
    {
      return id < m_sparse.size() &&  m_sparse[id] != INVALID_INDEX;
    }

    void remove(EntityID &id)
    {
      size_t index_remove = m_sparse[id];
      size_t last_index = m_dense.size() - 1;

      EntityID last_entity = m_entity_dense[last_index];
      m_dense[index_remove] = std::move(m_dense[last_index]);
      m_entity_dense[index_remove] = last_entity;
      m_sparse[last_entity] = index_remove;

      m_dense.pop_back();
      m_entity_dense.pop_back();

      m_sparse[id] = INVALID_INDEX;
    }
  private:
    static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();
    std::vector<T>        m_dense;
    std::vector<EntityID> m_entity_dense;
    std::vector<size_t>   m_sparse;
};

class Register 
{
  public:
    Entity create_entity()
    {
      EntityID id;
      if (!m_free_slots.empty())
      {
        id = m_free_slots.back();
        m_free_slots.pop_back();
        m_entity_dence[id] = id;
        return Entity(id);
      }
      else 
      {
        id = m_next_id ++;
        m_entity_dence.push_back(id);
      }
      return Entity(id);
    }

    void destroy_entity(Entity &entity)
    {
      if (entity.get_id() < m_entity_dence.size())
      {
        m_entity_dence[entity.get_id()] = NULL_ENTITY_ID;
        m_free_slots.push_back(entity.get_id());
      }
    }

    template <typename T>
      T& add(Entity entity, T component)
      {
        return get_pool<T>().insert_component(entity.get_id(), std::move(component));
      }
    template <typename T>
      T& get(Entity entity)
      {
        return get_pool<T>().get_component(entity.get_id());
      }
    template <typename T>
      bool has(Entity entity)
      {
        return get_pool<T>().has_component(entity.get_id());
      }
    template <typename T>
      void remove(Entity entity)
      {
        get_pool<T>().remove(entity.get_id());
      }
  private:

    template <typename T>
      IPool<T>& get_pool()
      {
        static IPool<T> pool;
        return pool;
      }

    EntityID m_next_id = 0;
    std::vector<EntityID> m_entity_dence;
    std::vector<size_t> m_free_slots;
};

