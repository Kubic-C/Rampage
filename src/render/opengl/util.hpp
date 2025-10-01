#pragma once

#include "../../common/common.hpp"

/* TGUI */
#include <TGUI/Backend/SDL-TTF-OpenGL3.hpp>
#include <TGUI/TGUI.hpp>

RAMPAGE_START

uint32_t createShaderFromSource(uint32_t shader_type, const char* src, int* size = nullptr);
uint32_t createShader(uint32_t shader_type, const char* file_path);

void GLAPIENTRY glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam);
void enableOpenglErrorCallback(IHost* host);

RAMPAGE_END
