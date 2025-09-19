#pragma once

#include "../commondef.hpp"

RAMPAGE_START

/**
 * Ensures a stable, incremental ID, regardless of Compilation Unit.
 * Works across dynamic libraries.
 *
 * The main object is owned by the host process and then
 * shared among its processes.
 */
template<typename IntT>
class StaticIdManager {
  template <typename T>
  constexpr static std::string_view typeName() {
#if defined(_MSC_VER)
    return __FUNCSIG__;       // MSVC
#else
    return __PRETTY_FUNCTION__; // GCC/Clang
#endif
  }
public:

  template<typename T>
  IntT id() {
    static IntT id = nextID<T>(); // Initialized once per unique build.
    return id;
  }

private:
  // This is called once per initialization per type per seperate compilation units..
  template<typename T>
  IntT nextID() {
    size_t idHash = std::hash<std::string_view>{}(typeName<T>());
    auto it = m_registry.find(idHash);
    if (it != m_registry.end()) {
      std::cout << "Found: " << boost::typeindex::type_id<T>().pretty_name() << " id: " << it->second << std::endl;
      return it->second;
    }

    int newId = ++m_counter;
    m_registry[idHash] = newId;
    std::cout << "New: " <<  boost::typeindex::type_id<T>().pretty_name()  << " id: " << newId << std::endl;
    return newId;
  }

public:
  IntT m_counter = 0;
  Map<size_t, IntT> m_registry;
};

RAMPAGE_END