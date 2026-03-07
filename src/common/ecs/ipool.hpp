#pragma once

#include "id.hpp"

RAMPAGE_START

template<typename T>
struct ObjectData {
#ifndef NDEBUG
  bool hasValue = false;
#endif

  std::array<u8, sizeof(T)> data;

  T* get() {
    assert(hasValue);
    return reinterpret_cast<T*>(data.data());
  }

  T* construct() {
#ifndef NDEBUG
    hasValue = true;
#endif
    return new (data.data()) T();
  }

  void destruct() {
    get()->~T();
#ifndef NDEBUG
    hasValue = false;
#endif
  }

  void move(ObjectData<T>* dst) {
    new (dst->data.data()) T(std::move(*get()));
    get()->~T();
#ifndef NDEBUG
    dst->hasValue = true;
    hasValue = false;
#endif
  }
};

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
  static IPool* createPool() {
    return new Pool();
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

  u8* get(EntityId id) override {
    assert(exists(id));
    return (u8*)m_objects.at(id);
  }

private:
  Map<EntityId, T*> m_objects;
  boost::object_pool<T> m_pool;
};

// TODO Make pool implementations private. Only expose the IPool interface and factory functions in IWorld.
template <typename T>
class SparsePool : public IPool {
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);

public:
  static IPool* createPool() {
    return new SparsePool<T>();
  }

  u8* create(EntityId id) override {
    assert(!exists(id));

    if (id >= m_sparse.size()) {
      size_t newSize = std::max((size_t)id + 1, m_sparse.size() * 2);
      m_sparse.resize(newSize, npos);
    }

    if (m_denseSize == m_denseCap) {
      allocateAdditional((m_denseCap + 1) * 2); // exponential
    }

    m_sparse[id] = m_denseSize;
    m_denseToEntity.push_back(id);
    m_dense[m_denseSize].construct();
    m_denseSize++;

    return get(id);
  }

  void destroy(EntityId id) override {
    assert(exists(id));

    size_t denseIdx = m_sparse[id];
    size_t lastIdx = m_denseSize - 1;

    m_dense[denseIdx].destruct();

    if (denseIdx != lastIdx) {
      m_dense[lastIdx].move(&m_dense[denseIdx]);
      EntityId lastEntity = m_denseToEntity[lastIdx];
      m_sparse[lastEntity] = denseIdx;
      m_denseToEntity[denseIdx] = lastEntity;
    }

    m_denseToEntity.pop_back();
    m_denseSize--;
    m_sparse[id] = npos;
  }

  bool exists(EntityId id) override {
    return m_sparse.size() > id && m_sparse[id] != npos;
  }

  u8* get(EntityId id) override {
    assert(exists(id));
    assert(m_dense[m_sparse[id]].hasValue);
    return reinterpret_cast<u8*>(m_dense[m_sparse[id]].get());
  }

protected:
  void allocateAdditional(size_t additionalSize) {
    size_t totalNewCap = m_denseCap + additionalSize;
    
    ObjectData<T>* newDense = static_cast<ObjectData<T>*>(std::malloc(totalNewCap * sizeof(ObjectData<T>)));
    if (m_dense) {
      for (size_t i = 0; i < m_denseSize; ++i)
        m_dense[i].move(newDense + i);
      std::free(m_dense);
    }

    m_denseCap = totalNewCap;
    m_dense = newDense;
  }

private:
  size_t m_denseCap = 0;
  size_t m_denseSize = 0;
  ObjectData<T>* m_dense = nullptr;
  std::vector<EntityId> m_denseToEntity;
  std::vector<size_t> m_sparse;
};

RAMPAGE_END
