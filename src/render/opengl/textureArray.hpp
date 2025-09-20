#pragma once

#include "../stb_image.h"
#include "util.hpp"

RAMPAGE_START

class TextureArray {
  public:
  TextureArray(u32 width, u32 height, u32 count) : m_width(width), m_height(height) {
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 4, GL_RGBA8, width, height, count);
  }

  void bind() { glBindTexture(GL_TEXTURE_2D_ARRAY, m_id); }

  void subWholeImage(u32 index, const void* data) {
    bind();
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                    data);
  }

  private:
  u32 m_width;
  u32 m_height;
  u32 m_id;
};

RAMPAGE_END