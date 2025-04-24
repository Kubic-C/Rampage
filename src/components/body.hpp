#pragma once

#include "../utility/base.hpp"

struct BodyComponent {
    b2BodyId id = b2_nullBodyId;

    ~BodyComponent() {
        b2DestroyBody(id);
    }
};
