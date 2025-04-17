#pragma once

#include "../utility/log.hpp"

inline uint32_t createShaderFromSource(uint32_t shader_type, const char* src, int* size = nullptr) {
  uint32_t shaderid;

  shaderid = glCreateShader(shader_type);
  glShaderSource(shaderid, 1, &src, size);
  glCompileShader(shaderid);

  {
    int success;
    char info_log[512];
    glGetShaderiv(shaderid, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shaderid, 512, NULL, info_log);
      printf("shader compilation failed: %s\n", info_log);
      glDeleteShader(shaderid);
      return -1;
    }
  }

  return shaderid;
}

inline uint32_t createShader(uint32_t shader_type, const char* file_path) {
  int file_size;
  char* file_data;

  {
    FILE* file;

    file = fopen(file_path, "r");
    if (!file) {
      return -1;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    file_data = (char*)malloc(file_size * sizeof(char));
    fread(file_data, sizeof(char), file_size, file);
    fclose(file);
  }

  return createShaderFromSource(shader_type, file_data, &file_size);
}

inline void GLAPIENTRY glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar* message, const void* userParam) {
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return;

  logGeneric("<bgWhite, fgBlue>OpenGL report:<reset>\n");
  logError(1, "\tsoure: 0x%x\n", source);
  logError(1, "\ttype: 0x%x\n", type);
  logError(1, "\tid: 0x%x\n", id);
  logError(1, "\tseverity: 0x%x\n", severity);
  logError(1, "\t%s\n", message);
}

inline void enableOpenglErrorCallback() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(glErrorCallback, nullptr);
}