#pragma once

#include "util.hpp"

RAMPAGE_START

class Sampler {
  public:
  Sampler() { glCreateSamplers(1, &m_id); }

  void parameter(GLenum name, GLenum param) { glSamplerParameteri(m_id, name, param); }

  void bind(u32 slot) { glBindSampler(slot, m_id); }

  private:
  u32 m_id;
};

RAMPAGE_END