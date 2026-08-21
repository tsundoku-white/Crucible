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
    T& insert_component(Entity &entity, T component)
    {
      if (has_component(entity.get_id()))
      {
        return get_component(get_component(entity.get_id()));
        std::print("id exists: id {}", entity.get_id());
      }
      if (get_component(entity.get_id()) > m_sparse.size())
        m_sparse.resize(entity.get_id() + 1, INVALID_INDEX);

      m_sparse[entity.get_id()] = m_sparse.size();
      m_entity_dense.push_back(entity.get_id());
      return m_dense.back();
    }

    T& get_component(Entity &entity)
    {
      return m_dense[m_sparse[entity.get_id()]];
    }

    bool has_component(Entity &entity)
    {
      return entity.get_id() < m_sparse.size() && 
        m_sparse[entity.get_id()] != INVALID_INDEX;
    }

    void remove(Entity &entity)
    {
      size_t index_remove = m_sparse[entity.get_id()];
      size_t last_index = m_dense.size() - 1;

      EntityID last_entity = m_entity_dense[last_index];
      m_dense[index_remove] = std::move(m_dense[last_index]);
      m_entity_dense[last_entity] = index_remove;

      m_dense.pop_back();
      m_entity_dense.pop_back();

      m_sparse[entity.get_id()] = INVALID_INDEX;
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
      if (!m_entity_dence.empty())
      {
        size_t index = m_free_slots.back();
        m_free_slots.pop_back();
        m_entity_dence[index] = id;
        return Entity(NULL_ENTITY_ID);
      }
      else 
      {
        id = m_next_id ++;
      }
      return Entity(id);
    }

    void destroy_entity(Entity &entity)
    {
      if (entity.get_id() >= m_entity_dence.size())
      {
        m_entity_dence[entity.get_id()];
        m_free_slots.push_back(entity.get_id());
      }
    }

    template <typename T>
      T& add(Entity entity, T component)
      {
        return get_pool<T>().insert_component(entity, std::move(component));
      }
    template <typename T>
      T& get(Entity entity, T component)
      {
        return get_pool<T>().get_component(entity);
      }
    template <typename T>
      T& has(Entity entity, T component)
      {
        return get_pool<T>().has_component(entity);
      }
    template <typename T>
      T& remove(Entity entity, T component)
      {
        return get_pool<T>().remove(entity);
      }


  private:
    template <typename T>
      IPool<T>& get_pool();

    EntityID m_next_id;
    std::vector<EntityID> m_entity_dence;
    std::vector<size_t> m_free_slots;
};

