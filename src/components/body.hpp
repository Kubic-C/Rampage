#pragma once

#include "../utility/base.hpp"

struct BodyComponent {
    b2BodyId id = b2_nullBodyId;

    BodyComponent() = default;
    BodyComponent(const BodyComponent& body) = default;

    BodyComponent(BodyComponent&& other) noexcept {
      id = other.id;
      other.id = b2_nullBodyId;
    }

    BodyComponent& operator=(BodyComponent&& other) noexcept {
      id = other.id;
      other.id = b2_nullBodyId;

      return *this;
    }

    ~BodyComponent() {
        if (b2Body_IsValid(id)) {
          b2DestroyBody(id);
        }
    }
};
