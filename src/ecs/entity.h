#pragma once

using EntityID = std::uint64_t;
static constexpr EntityID INVALID_ENTITY_ID = std::numeric_limits<uint64_t>::max();

struct Entity
{
  explicit Entity(EntityID id) : m_id(id) {}

  bool      isValid()     const { return m_id != INVALID_ENTITY_ID; }
  EntityID  getId()       const { return m_id; }
  void      invalidate()  { m_id = INVALID_ENTITY_ID; }

  private:
  EntityID m_id;
};
