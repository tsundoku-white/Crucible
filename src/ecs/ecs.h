#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <limits>

using EntityID    = std::uint64_t;
using ComponentID = std::uint8_t;

constexpr EntityID NULL_ENTITY_ID = std::numeric_limits<EntityID>::max();

struct Entity
{
  explicit Entity(EntityID id) : m_id(id) {}

  EntityID id()      const { return m_id;                   }
  bool     is_null() const { return m_id == NULL_ENTITY_ID; }

  private:
  EntityID m_id;
};

template<typename T>
class ComponentPool 
{
  public:
    T& insert(EntityID id, T component)
    {
      // check dup
      if (has(id))
        return  get(id);

      // grows the m_sparse container to fit the component
      if (id >= m_sparse.size())
        m_sparse.resize(id + 1, INVALID_INDEX);

      // point space for for new dence index
      m_sparse[id] = m_dense.size();

      // push the componet in dence entity array
      m_dense_entities.push_back(id);

      // push to the component array
      m_dense.push_back(std::move(component));

      // return a ref to entity
      return m_dense.back();
    }
    T& get(EntityID id)
    {
      // returns the dence index based on the comment and ref to id
        return m_dense[m_sparse[id]];
    }
    bool has(EntityID id)
    {
      // return true if both the id is grater than the size of the sparse array
      // and the sparse id is not equal to INVALID_INDEX
      return id < m_sparse.size() && m_sparse[id] != INVALID_INDEX;
    }
    void remove(EntityID id)
    {
      if (!has(id)) return;
      // copy a local ref to m_sparse
      size_t index_to_remove = m_sparse[id];

      // making a index value
      size_t last_index = m_dense.size() - 1;

      // copy of the index with the one removed
      EntityID last_entity = m_dense_entities[last_index];

      // updating the m_dense witht he right list
      m_dense[index_to_remove] = std::move(m_dense[last_index]);

      // moving evering to its still compact
      m_dense_entities[index_to_remove] = last_entity;

      m_sparse[last_entity] = index_to_remove;

      // removing the data form the array m_dense
      m_dense.pop_back();
      m_dense_entities.pop_back();

      // making id invalid
      m_sparse[id] = INVALID_INDEX;
    }

  private:
    static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();
    std::vector<T>        m_dense; // packed array of conponets
    std::vector<EntityID> m_dense_entities; // a parsial array tha tis owned by m_dense 
    std::vector<size_t>   m_sparse; // more loose place holder of the entitys to later be conpacted in dence
};

class ECS
{
  public:
    Entity create_entity()
    {
      EntityID id;
      if (!m_available_entities.empty())
      {
        id = m_available_entities.back();
        m_available_entities.pop_back();
      }
      else 
      {
        id = m_next_id++;
      }
      return Entity(id);
    }

    void destroy_entity(Entity &entity)
    {
      if (!entity.is_null())
      {
        m_available_entities.push_back(entity.id());
      }
    }

    template<typename T>
    T& add_component(Entity ent , T component)
    {
      return get_pool<T>().insert(ent.id(), component);
    }
    template<typename T>
    T& get_component(Entity ent, T component)
    {
      return get_pool<T>().get(ent.id());
    }
    template<typename T>
    T& has_component(Entity ent, T component)
    {
      return get_pool<T>().has(ent.id());
    }
    template<typename T>
    T& remove_component(Entity ent , T component)
    {
      return get_pool<T>().remove(ent.id());
    }
  private:

    template<typename T>
      ComponentPool<T>& get_pool()
      {
        static ComponentPool<T> pool;
        return pool;
      }

    EntityID m_next_id = 0;
    std::vector<EntityID> m_available_entities; 
};
