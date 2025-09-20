#pragma once

#include "util.hpp"

RAMPAGE_START

class Shader {
  public:
  Shader() : m_id(glCreateProgram()) {}

  ~Shader() { glDeleteProgram(m_id); }

  bool loadShaderFiles(const char* vsh_path, const char* fsh_path, std::string& res) {
    uint32_t vsh, fsh;
    bool vsh_invalid, fsh_invalid, leave;

    vsh = createShader(GL_VERTEX_SHADER, vsh_path);
    fsh = createShader(GL_FRAGMENT_SHADER, fsh_path);

    vsh_invalid = vsh == UINT32_MAX;
    fsh_invalid = fsh == UINT32_MAX;
    leave = vsh_invalid || fsh_invalid;

    if (fsh_invalid) {
      glDeleteShader(vsh);
    }

    if (vsh_invalid) {
      glDeleteShader(fsh);
    }

    if (leave) {
      return false;
    }

    if (!loadShaderModules(vsh, fsh, true, res)) {
      return false;
    }

    return true;
  }

  bool loadShaderStr(const char* vs, const char* fs, std::string& res) {
    uint32_t vs_shader = createShaderFromSource(GL_VERTEX_SHADER, vs, nullptr);
    uint32_t fs_shader = createShaderFromSource(GL_FRAGMENT_SHADER, fs, nullptr);

    if (vs_shader == UINT32_MAX || fs_shader == UINT32_MAX)
      return false;

    return loadShaderModules(vs_shader, fs_shader, true, res);
  }

  bool loadShaderModules(uint32_t vsh, uint32_t fsh, bool delete_shaders, std::string& result) {
    bool ret = true;

    glAttachShader(m_id, vsh);
    glAttachShader(m_id, fsh);
    glLinkProgram(m_id);

    {
      int success;
      char info_log[512];
      glGetProgramiv(m_id, GL_LINK_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(m_id, 512, nullptr, info_log);
        result += info_log;
        result += "\n\n";
        ret = false;
      }
    }

    if (delete_shaders) {
      glDeleteShader(vsh);
      glDeleteShader(fsh);
    }

    return ret;
  }

  void use() { glUseProgram(m_id); }

  void setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
  }

  void setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
  }

  void setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
  }

  void setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
  }

  void setVec2(const std::string& name, float x, float y) const {
    glUniform2f(glGetUniformLocation(m_id, name.c_str()), x, y);
  }

  void setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
  }

  void setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(m_id, name.c_str()), x, y, z);
  }

  void setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
  }

  void setVec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(m_id, name.c_str()), x, y, z, w);
  }

  void setMat2(const std::string& name, const glm::mat2& mat) const {
    glUniformMatrix2fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }

  void setMat3(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }

  void setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }

  private:
  uint32_t m_id;
};

RAMPAGE_END