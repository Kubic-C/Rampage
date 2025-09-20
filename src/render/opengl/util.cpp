#include "util.hpp"

RAMPAGE_START

uint32_t createShaderFromSource(uint32_t shader_type, const char* src, int* size) {
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

uint32_t createShader(uint32_t shader_type, const char* file_path) {
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
    if (file_data == 0)
      return 0;
    fread(file_data, sizeof(char), file_size, file);
    fclose(file);
  }

  return createShaderFromSource(shader_type, file_data, &file_size);
}

void GLAPIENTRY glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                       const GLchar* message, const void* userParam) {
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return;

  IHost& host = *(IHost*)(userParam);

  host.log("<bgWhite, fgBlue>OpenGL report:<reset>\n");
  host.log(1, "\tsoure: 0x%x\n", source);
  host.log(1, "\ttype: 0x%x\n", type);
  host.log(1, "\tid: 0x%x\n", id);
  host.log(1, "\tseverity: 0x%x\n", severity);
  host.log(1, "\t%s\n", message);
}

void enableOpenglErrorCallback(IHost* host) {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(glErrorCallback, host);
}

RAMPAGE_END