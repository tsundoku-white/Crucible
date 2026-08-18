#pragma once

#include <cstdint>
#include <vector>
struct Entity
{
  uint32_t id;
};

template< typename T> 

class ECS 
{
  public:
    Entity create();

  private:
    std::vector<ECS> m_entities;
    uint32_t m_next_id = 0;;
};
