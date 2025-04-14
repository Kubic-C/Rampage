#pragma once

#include "../tile.hpp"
#include "camera.hpp"
#include "opengl.hpp"
#include "baseRender.hpp"

class Render {
public:
  static constexpr int MinimumMajorGLVersion = 4;
  static constexpr int MinimumMinorGLVersion = 0;

  static SDL_WindowFlags getNecessaryWindowFlags() {
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    return flags;
  }

public:
  Render(SDL_Window* window, Entity camera)
    : m_window(window), m_camera(camera), m_status(Status::Ok) {
    /* Basic state */
#ifndef NDEBUG
    enableOpenglErrorCallback();
#endif 

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    resizeViewportToScreenDim();

    camera.world().component<IsRender>();
  }

  Status getStatus() {
    return m_status;
  }

  void mesh() {
    /* collect active renderers */
    m_renderers.clear();
    EntityWorld& world = m_camera.world();
    EntityIterator it = world.getWith(world.set<IsRender>());
    while (it.hasNext()) {
      Entity renderer = it.next();

      Module* module = renderer.get<EntityWorld::ModuleData>().m_module.get();
      BaseRenderModule* renderData = dynamic_cast<BaseRenderModule*>(module);
      auto it = std::lower_bound(m_renderers.begin(), m_renderers.end(), renderData, 
        [](BaseRenderModule* first, BaseRenderModule* other)
        {
          return first->getPriority() < other->getPriority();
        });
      m_renderers.insert(it, renderData);
    }

    for (auto renderer : m_renderers) {
      renderer->preMesh();
    }

    for (auto renderer : m_renderers) {
      renderer->onMesh();
    }
  }

  void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto renderer : m_renderers) {
      renderer->preRender();
    }
    
    glm::mat4 vp = m_proj * getView();
    for (auto renderer : m_renderers) {
      renderer->onRender(vp);
    }

    for (auto renderer : m_renderers) {
      renderer->postRender();
    }

    SDL_GL_SwapWindow(m_window);
  }

  void setCamera(Entity camera) {
    m_camera = camera;
  }

  glm::mat4 getView() {
    glm::ivec2 screenDim;
    SDL_GetWindowSize(m_window, &screenDim.x, &screenDim.y);
    CameraComponent& camera = m_camera.get<CameraComponent>();
    PosComponent pos = m_camera.get<PosComponent>();
    RotComponent rot = m_camera.get<RotComponent>();

    return camera.view(Transform(pos, camera.m_rot), (glm::vec2)screenDim);
  }

  void resizeViewport(const glm::ivec2& size) {
    glm::vec2 fsize = (glm::vec2)size * 0.5f;
    m_proj = glm::ortho<float>(-fsize.x, fsize.x, -fsize.y, fsize.y);
    glViewport(0, 0, size.x, size.y);
  }

  void resizeViewportToScreenDim() {
    glm::ivec2 screenSize;
    SDL_GetWindowSize(m_window, &screenSize.x, &screenSize.y);

    resizeViewport(screenSize);
  }

  glm::vec2 getWorldCoords(const glm::ivec2& _screenCoords) {
    glm::ivec2 screenSize;
    SDL_GetWindowSize(m_window, &screenSize.x, &screenSize.y);

    glm::vec2 screenCoords = { (float)_screenCoords.x, (float)_screenCoords.y };

    glm::mat4 view = getView();

    screenCoords.y = screenSize.y - screenCoords.y;

    glm::vec2 worldCoords =
      glm::unProject(
        glm::vec3(screenCoords, -1.0f),
        view,
        m_proj,
        glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

    return worldCoords;
  }

  glm::vec2 getScreenCoords(const glm::ivec2& worldCoords) {
    glm::ivec2 screenSize;
    SDL_GetWindowSize(m_window, &screenSize.x, &screenSize.y);

    glm::mat4 view = getView();

    glm::vec2 screenCoords =
      glm::project(
        glm::vec3(screenCoords, -1.0f),
        view,
        m_proj,
        glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

    screenCoords.y = screenSize.y - screenCoords.y;

    return screenCoords;
  }

private:
  Status m_status;
  glm::mat4 m_proj;
  Entity m_camera;
  std::vector<BaseRenderModule*> m_renderers;
  SDL_Window* m_window;
};