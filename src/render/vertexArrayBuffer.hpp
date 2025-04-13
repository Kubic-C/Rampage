#pragma once

#include "vertexBuffer.hpp"

class VertexArrayBuffer {
public:
  static void unbind() {
    glBindVertexArray(0);
  }

public:
  VertexArrayBuffer() {
    glCreateVertexArrays(1, &m_id);
  }

  VertexArrayBuffer(VertexArrayBuffer&& other) {
    m_id = other.m_id;
    other.m_id = 0;
  }

  ~VertexArrayBuffer() {
    if (isValid()) {
      glDeleteVertexArrays(1, &m_id);
    }
  }

  bool isValid() {
    return m_id != 0;
  }

  void addVertexArrayAttrib(const VertexBuffer& vertexBuffer, u32 slot, u32 size, GLenum type, bool normalized, u32 stride, u32 offset) {
    bind();
    vertexBuffer.bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(slot, size, type, (int)normalized, stride, (const void*)offset);
    glEnableVertexAttribArray(slot);
    VertexBuffer::unbind(GL_ARRAY_BUFFER);
    VertexArrayBuffer::unbind();
  }
  
  void bind() {
    glBindVertexArray(m_id);
  }

private:
  u32 m_id;
};