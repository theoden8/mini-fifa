#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "Debug.hpp"
#include "Logger.hpp"
#include "ShaderProgram.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"

struct Vertex {
  glm::vec3 pos;
  glm::vec3 nrm;
  glm::vec2 txcoords;
  glm::vec3 tg;
  glm::vec3 tg2;
};

struct Texture {
  GLuint id;
  std::string type;
  std::string path;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<unsigned> indices;
  std::vector<Texture> textures;

  gl::VertexArray vao;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned> indices, std::vector<Texture> textures):
    vertices(vertices), indices(indices), textures(textures)
  {}

  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC3> vbo;
  gl::Attrib<GL_ELEMENT_ARRAY_BUFFER, gl::AttribType::VEC3> ebo;

  void init() {
    vao.init();
    vbo.init(); ebo.init();
    vao.bind();
    vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW); GLERROR
    ebo.bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW); GLERROR
    vao.enable(ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr); GLERROR
    vao.enable(ebo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nrm));
    vao.enable(ebo);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, txcoords));
    vao.enable(ebo);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tg));
    vao.enable(ebo);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tg2));
    /* decltype(ebo)::unbind(); */
    decltype(vao)::unbind();
  }

  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program) {
    program.use();
    unsigned
      diffuseNr = 1,
      specNr = 1,
      normalNr = 1,
      heightNr = 1;
    for(unsigned i = 0; i < textures.size(); ++i) {
      gl::Texture::set_active(i);
      std::string number;
      std::string name = textures[i].type;
      if(name == "texture_diffuse") {
        number = std::to_string(diffuseNr++);
      } else if(name == "texture_specular") {
        number = std::to_string(specNr++);
      } else if(name == "texture_normal") {
        number = std::to_string(normalNr++);
      } else if(name == "texture_height") {
        number = std::to_string(heightNr++);
      }
      /* Logger::Info("Mesh: field %s\n", (name + number).c_str()); */
      glUniform1i(glGetUniformLocation(program.id(), (name + number).c_str()), i); GLERROR
      glBindTexture(GL_TEXTURE_2D, textures[i].id); GLERROR
    }
    vao.bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); GLERROR
    gl::VertexArray::unbind();
    std::remove_reference_t<decltype(program)>::unuse();
  }

  void clear() {
    vbo.clear();
    ebo.clear();
    vao.clear();
  }
};
