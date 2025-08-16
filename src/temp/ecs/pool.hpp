#pragma once

#include "id.hpp"

class IPool {
  public:
  virtual ~IPool() = default;

  virtual u8* create(EntityId id) = 0;
  virtual void destroy(EntityId id) = 0;
  virtual bool exists(EntityId id) = 0;
  virtual u8* get(EntityId id) = 0;

  template <typename T>
  T& get(EntityId id) {
    return *(T*)get(id);
  }
};

template <typename T>
class Pool : public IPool {
  public:
  static IPool* createPool() { return new Pool(); }

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

  bool exists(EntityId id) override { return m_objects.find(id) != m_objects.end(); }

  u8* get(EntityId id) override {
    assert(exists(id));
    return (u8*)m_objects.at(id);
  }

  private:
  Map<EntityId, T*> m_objects;
  boost::object_pool<T> m_pool;
};

/* Component pointers and references may be invalidated*/
template <typename T>
class SparsePool : public IPool {
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

  public:
  static IPool* createPool() { return new SparsePool<T>(); }

  u8* create(EntityId id) override {
    assert(!exists(id));

    if (id >= m_sparse.size()) {
      size_t newSize = std::max((size_t)id + 1, m_sparse.size() * 2);
      m_sparse.resize(newSize, npos);
    }

    if (!m_freed.empty()) {
      size_t freeIndex = m_freed.back();

      m_dense[freeIndex].emplace();
      m_sparse[id] = freeIndex;
      m_freed.pop_back();
    } else {
      m_dense.emplace_back(std::in_place);
      m_sparse[id] = m_dense.size() - 1;
    }

    return get(id);
  }

  void destroy(EntityId id) override {
    assert(exists(id));

    size_t remIndex = m_sparse[id];
    m_dense[remIndex].reset();
    m_freed.push_back(remIndex);
    m_sparse[id] = npos;
  }

  bool exists(EntityId id) override { return m_sparse.size() > id && m_sparse[id] != npos; }

  u8* get(EntityId id) override {
    assert(exists(id));
    assert(m_dense[m_sparse[id]].has_value());
    return reinterpret_cast<u8*>(&m_dense[m_sparse[id]].value());
  }

  private:
  std::vector<size_t> m_freed;
  std::vector<std::optional<T>> m_dense;
  std::vector<size_t> m_sparse;
};
