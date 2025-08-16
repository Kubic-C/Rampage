#pragma once

#include "vertexBuffer.hpp"

template <typename VertexType, size_t verticePerRender, size_t verticesPerSubdata>
struct Mesh {
  VertexBuffer buffer;
  uint32_t verticesToRender = 0;
  uint32_t verticesOnBuffer = 0;

  Mesh() { buffer.resize(GL_ARRAY_BUFFER, sizeof(VertexType) * 1024); }

  Mesh(const Mesh<VertexType, verticePerRender, verticesPerSubdata>& other) {
    buffer = other.buffer;
    verticesToRender = other.verticesToRender;
    verticesOnBuffer = other.verticesOnBuffer;
  }

  void reset() {
    verticesToRender = 0;
    verticesOnBuffer = 0;
  }

  void addMesh(VertexType* mesh) {
    if (buffer.getSize() < (verticesOnBuffer + verticesPerSubdata) * sizeof(VertexType))
      buffer.resize(GL_ARRAY_BUFFER, (verticesOnBuffer + verticesPerSubdata) * sizeof(VertexType) * 2);

    buffer.subBufferData(verticesOnBuffer * sizeof(VertexType), sizeof(VertexType) * verticesPerSubdata,
                         mesh);
    verticesOnBuffer += verticesPerSubdata;
    verticesToRender += verticePerRender;
  }
};
