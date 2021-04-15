#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "Debug.hpp"
#include "Logger.hpp"
#include "ShaderProgram.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"


struct ModelVertex {
  glm::vec3 pos;
  glm::vec3 nrm;
  glm::vec2 txcoords;
  glm::vec3 tangent;
  glm::vec3 bitan;
};

struct ModelTexture {
  GLuint id;
  std::string type;
  std::string path;
};

struct Mesh {
  std::vector<ModelVertex> vertices;
  std::vector<GLuint> indices;
  std::vector<ModelTexture> textures;

  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC3> vbo;
  gl::Buffer<GL_ELEMENT_ARRAY_BUFFER, gl::BufferElementType::VEC3> ebo;

  gl::Attrib<decltype(ebo)>
    apos,
    anrm,
    atxc,
    atng,
    abtg;

  gl::VertexArray<
    decltype(apos),
    decltype(anrm),
    decltype(atxc),
    decltype(atng),
    decltype(abtg)
  > vao;

  using ShaderBufferVBO = decltype(vbo);
  using ShaderBufferEBO = decltype(ebo);
  using VertexArray = decltype(vao);

  Mesh(const std::vector<ModelVertex> &vertices, const std::vector<GLuint> &indices, const std::vector<ModelTexture> &textures):
    vertices(vertices), indices(indices), textures(textures),
    apos("aPos", ebo),
    anrm("aNormal", ebo),
    atxc("aTexCoords", ebo),
    atng("aTangent", ebo),
    abtg("aBiTangent", ebo),
    vao(apos, anrm, atxc, atng, abtg)
  {}

  void init() {
    VertexArray::init(vao);
    VertexArray::bind(vao);

    ShaderBufferVBO::init(vbo);
    ShaderBufferEBO::init(ebo);

    ShaderBufferVBO::bind(vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), &vertices[0], GL_STATIC_DRAW); GLERROR

    ShaderBufferEBO::bind(ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW); GLERROR

    {
      vao.bind();
      vao.enable(apos);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), nullptr); GLERROR
      vao.enable(anrm);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, nrm)); GLERROR
      vao.enable(atxc);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, txcoords)); GLERROR
      vao.enable(atng);
      glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, tangent)); GLERROR
      vao.enable(abtg);
      glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, bitan)); GLERROR
      vao.unbind();
    }

    /* ShaderBufferVBO::unbind(); */
    /* ShaderBufferEBO::unbind(); */
    VertexArray::unbind();
  }

  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program) {
    using ShaderProgram = gl::ShaderProgram<ShaderTs...>;

    ShaderProgram::use(program);
    GLuint
      diffuseNr = 1,
      specNr = 1,
      normalNr = 1,
      heightNr = 1;
    for(GLuint i = 0; i < textures.size(); ++i) {
      int number = 0;
      const std::string &name = textures[i].type;
      if(name == "texture_diffuse") {
        number = (diffuseNr++);
      } else if(name == "texture_specular") {
        number = (specNr++);
      } else if(name == "texture_normal") {
        number = (normalNr++);
      } else if(name == "texture_height") {
        number = (heightNr++);
      } else {
        TERMINATE("unregistered shader attribute");
      }
      /* Logger::Info("Mesh: field %s\n", (name + number).c_str()); */
      gl::Uniform<gl::UniformType::SAMPLER2D> uSampler((name + std::to_string(number)).c_str());
      uSampler.set_id(program.id());
      uSampler.set_data(i);
      gl::Texture::set_active(i);
      gl::Texture::bind(textures[i].id);
    }
    VertexArray::bind(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); GLERROR
    VertexArray::unbind();
    ShaderProgram::unuse();
  }

  void clear() {
    apos.clear();
    anrm.clear();
    atxc.clear();
    atng.clear();
    abtg.clear();
    ShaderBufferVBO::clear(vbo);
    ShaderBufferEBO::clear(ebo);
    VertexArray::clear(vao);
  }
};
