#pragma once

#include "util.hpp"

class VertexBuffer {
  public:
  static void unbind(GLenum target) { glBindBuffer(target, 0); }

  public:
  VertexBuffer() { glCreateBuffers(1, &m_id); }

  VertexBuffer(VertexBuffer&& other) noexcept {
    m_id = other.m_id;
    other.m_id = 0;
  }

  ~VertexBuffer() {
    if (isValid()) {
      glDeleteBuffers(1, &m_id);
    }
  }

  VertexBuffer& operator=(VertexBuffer&& other) noexcept {
    m_id = other.m_id;
    other.m_id = 0;

    return *this;
  }

  void unmapBuffer() { glUnmapNamedBuffer(m_id); }

  void* mapBufferRange(int offset, int length, int access) {
    return glMapNamedBufferRange(m_id, offset, length, access);
  }

  void* mapBuffer(int access) { return glMapNamedBuffer(m_id, access); }

  void bufferData(u32 size, void* data, GLenum usage) const { glNamedBufferData(m_id, size, data, usage); }

  void bufferStorage(u32 size, void* data, GLenum usage) const {
    glNamedBufferStorage(m_id, size, data, usage);
  }

  void subBufferData(u32 offset, u32 size, void* data) const {
    glNamedBufferSubData(m_id, offset, size, data);
  }

  void bind(GLenum target) const { glBindBuffer(target, m_id); }

  bool isValid() const { return m_id != 0; }

  int getSize() {
    int size;
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    return size;
  }

  void resize(GLenum type, size_t new_size) {
    uint32_t copy_buffer;
    int old_size;


    // 1) copy data over to the copy buffer
    glBindBuffer(GL_COPY_READ_BUFFER, m_id);
    glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &old_size);

    if (old_size == 0) {
      glBindBuffer(type, m_id);
      glBufferData(type, new_size, nullptr, GL_DYNAMIC_DRAW);
      return;
    }

    glCreateBuffers(1, &copy_buffer);

    glBindBuffer(GL_COPY_WRITE_BUFFER, copy_buffer);
    glBufferData(GL_COPY_WRITE_BUFFER, old_size, nullptr, GL_STATIC_COPY);

    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, old_size);

    // 2) resize the main buffer
    glBindBuffer(GL_COPY_WRITE_BUFFER, m_id);
    glBindBuffer(GL_COPY_READ_BUFFER, copy_buffer);
    glBufferData(GL_COPY_WRITE_BUFFER, new_size, nullptr, GL_DYNAMIC_DRAW);

    // 3) then write back to the main buffer and cleanup
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, old_size);
    glDeleteBuffers(1, &copy_buffer);

    // reset the state
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
  }

  private:
  u32 m_id;
};
