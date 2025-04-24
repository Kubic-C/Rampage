#pragma once

#include "id.hpp"

class IPool {
public:
  virtual u8* create(EntityId id) = 0;
  virtual void destroy(EntityId id) = 0;
  virtual bool exists(EntityId id) = 0;
  virtual u8* get(EntityId id) = 0;

  template<typename T>
  T& get(EntityId id) {
    return *(T*)get(id);
  }
};

template<typename T>
class Pool : public IPool {
public:
  static std::shared_ptr<IPool> createPool() {
    return std::make_shared<Pool<T>>();
  }

  u8* create(EntityId id) override {
    assert(!exists(id));
    m_objects[id] = m_pool.construct();
    return (u8*)m_objects.find(id)->second;
  }

  void destroy(EntityId id) override {
    assert(exists(id));
    m_pool.destroy(m_objects.find(id)->second);
    m_objects.erase(id);
  }

  bool exists(EntityId id) override {
    return m_objects.find(id) != m_objects.end();
  }

  u8* get(EntityId id)override {
    assert(exists(id));
    return (u8*)m_objects.at(id);
  }

private:
  Map<EntityId, T*> m_objects;
  boost::object_pool<T> m_pool;
};