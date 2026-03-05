#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace Rampson {

// Describes a single validation failure with its JSON path and message.
struct ValidationError {
  std::string path;
  std::string message;
};

// Aggregates validation errors from a validate() call.
struct ValidationResult {
  std::vector<ValidationError> errors;

  bool valid() const { return errors.empty(); }

  void add(const std::string& path, const std::string& message) {
    errors.push_back({path, message});
  }

  void merge(const ValidationResult& other) {
    errors.insert(errors.end(), other.errors.begin(), other.errors.end());
  }

  std::string summary() const {
    if (errors.empty()) return "Validation passed.";
    std::ostringstream os;
    os << errors.size() << " validation error(s):\n";
    for (const auto& e : errors) {
      os << "  " << e.path << ": " << e.message << "\n";
    }
    return os.str();
  }
};

// Polymorphic base for all Rampson-generated struct types.
// Enables safe downcasting via dynamic_cast / as<T>().
class JsonValue {
public:
  virtual ~JsonValue() = default;

  template <typename T>
  T* as() { return dynamic_cast<T*>(this); }

  template <typename T>
  const T* as() const { return dynamic_cast<const T*>(this); }

  template <typename T>
  bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
};

} // namespace rmp::json
