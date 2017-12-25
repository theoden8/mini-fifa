#pragma once

#include <array>

#include "Window.hpp"
#include "Transformation.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"
#include "ShaderUniform.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"

struct Region {
  glm::vec2 xs, ys;
  Region(glm::vec2 xs, glm::vec2 ys):
    xs(xs), ys(ys)
  {
    if(xs.x>xs.y)xs=glm::vec2(xs.y,xs.x);
    if(ys.x>ys.y)ys=glm::vec2(ys.y,ys.x);
  }

  bool contains(float x, float y) const {
    return
      xs.x <= x && x <= xs.y
      && ys.x <= y && y <= ys.y;
  }
};

struct Pitch {
  Transformation transform;
  glm::mat4 matrix;

  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertex;
  gl::Texture grassTx;

  Pitch():
    program({"pitch.vert", "pitch.frag"}),
    uTransform("transform"),
    attrVertex("vertex"),
    grassTx("grass")
  {
    transform.SetScale(2, 1, 1);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(0, 1, 0, 0.f);
  }

  void init() {
    attrVertex.init();
    attrVertex.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      1,1, -1,1, -1,-1,
      -1,-1, 1,-1, 1,1,
    });

    vao.init();
    vao.bind();
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0, 0);
    gl::VertexArray::unbind();
    program.init(vao, {"attrVertex"});
    grassTx.init("assets/grass.png");

    grassTx.uSampler.set_id(program.id());
    uTransform.set_id(program.id());
  }

  Region playable_area() {
    return Region(glm::vec2(-1.80, 1.80), glm::vec2(-.95, .95));
  }

  Region left_half() {
    auto playable = playable_area();
    return Region(glm::vec2(playable.xs.x, 0), glm::vec2(playable.ys.x, playable.ys.y));
  }

  Region right_half() {
    auto playable = playable_area();
    return Region(glm::vec2(0, playable.xs.y), glm::vec2(playable.ys.x, playable.ys.y));
  }

  /* void keyboard(GLFWwindow *w) { */
    /* if(glfwGetKey(w, GLFW_KEY_UP)) { */
    /*   transform.MovePosition(0, -.05, 0); */
    /* } else if(glfwGetKey(w, GLFW_KEY_DOWN)) { */
    /*   transform.MovePosition(0, .05, 0); */
    /* } else if(glfwGetKey(w, GLFW_KEY_LEFT)) { */
    /*   transform.MovePosition(.05, 0, 0); */
    /* } else if(glfwGetKey(w, GLFW_KEY_RIGHT)) { */
    /*   transform.MovePosition(-.05, 0, 0); */
    /* } else if(glfwGetKey(w, GLFW_KEY_W)) { */
    /*   transform.Rotate(1, 0, 0, 10); */
    /* } else if(glfwGetKey(w, GLFW_KEY_S)) { */
    /*   transform.Rotate(1, 0, 0, -10); */
    /* } else if(glfwGetKey(w, GLFW_KEY_A)) { */
    /*   transform.Rotate(0, 1, 0, 10); */
    /* } else if(glfwGetKey(w, GLFW_KEY_D)) { */
    /*   transform.Rotate(0, 1, 0, -10); */
    /* } else if(glfwGetKey(w, GLFW_KEY_Q)) { */
    /*   transform.Rotate(0, 0, 1, 10); */
    /* } else if(glfwGetKey(w, GLFW_KEY_E)) { */
    /*   transform.Rotate(0, 0, 1, -10); */
    /* } else if(glfwGetKey(w, GLFW_KEY_EQUAL)) { */
    /*   transform.Scale(1.05); */
    /* } else if(glfwGetKey(w, GLFW_KEY_MINUS)) { */
    /*   transform.Scale(1./1.05); */
    /* } else if(glfwGetKey(w, GLFW_KEY_0)) { */
    /*   transform.SetScale(1.0f); */
    /*   transform.SetPosition(0, 0, .1); */
    /*   transform.SetRotation(0, 1, 0, 0); */
    /* } */
  /* } */

  void display(Camera &cam) {
    program.use();

    if(transform.has_changed || cam.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    gl::Texture::set_active(0);
    grassTx.bind();
    grassTx.set_data(0);

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    decltype(program)::unuse();
  }

  void clear() {
    grassTx.clear();
    attrVertex.clear();
    vao.clear();
    program.clear();
  }
};
