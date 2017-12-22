#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "incgraphics.h"
#include "Debug.hpp"

namespace gl {
enum class AttribType {
  INTEGER, FLOAT,
  VEC2, VEC3, VEC4,
  IVEC2, IVEC3, IVEC4,
  MAT2, MAT3, MAT4
};

template <AttribType A> struct a_cast_type { using type = void; };
template <> struct a_cast_type <AttribType::INTEGER> { using type = GLint; using vtype = GLint; };
template <> struct a_cast_type <AttribType::FLOAT> { using type = GLfloat; using vtype  = GLfloat; };
template <> struct a_cast_type <AttribType::VEC2> { using type = glm::vec2; using vtype  = GLfloat; };
template <> struct a_cast_type <AttribType::VEC3> { using type = glm::vec3; using vtype  = GLfloat; };
template <> struct a_cast_type <AttribType::VEC4> { using type = glm::vec4; using vtype  = GLfloat; };
template <> struct a_cast_type <AttribType::IVEC2> { using type = glm::ivec2; using vtype  = GLint; };
template <> struct a_cast_type <AttribType::IVEC3> { using type = glm::ivec3; using vtype  = GLint; };
template <> struct a_cast_type <AttribType::IVEC4> { using type = glm::ivec4; using vtype  = GLint; };
template <> struct a_cast_type <AttribType::MAT2> { using type = glm::mat2; using vtype  = GLfloat; };
template <> struct a_cast_type <AttribType::MAT3> { using type = glm::mat3; using vtype  = GLfloat; };
template <> struct a_cast_type <AttribType::MAT4> { using type = glm::mat4; using vtype  = GLfloat; };

template <typename T> struct gl_scalar_enum { static constexpr GLenum value = 0; };
template <> struct gl_scalar_enum<GLint> { static constexpr GLenum value = GL_INT; };
template <> struct gl_scalar_enum<GLfloat> { static constexpr GLenum value = GL_FLOAT; };

template <typename T>
struct AttribBufferAccessor {
  T attrib;
  size_t i;

  AttribBufferAccessor(T attrib, size_t i):
    attrib(attrib), i(i)
  {}

  using atype = a_cast_type<T::attrib_type>;
  AttribBufferAccessor operator=(typename atype::type vec) {
    attrib.bind();
    glBufferSubData(
      T::array_type,
      i*sizeof(typename atype::type),
      sizeof(typename atype::vtype),
      glm::value_ptr(vec)
    ); GLERROR
  }
};

struct GenericShaderAttrib {};
template <GLenum ArrayType, AttribType AttribT>
struct Attrib : GenericShaderAttrib {
  static constexpr GLenum array_type = ArrayType;
  static constexpr AttribType attrib_type = AttribT;
  std::string location = "";
  GLuint vbo = 0;

  Attrib(){}
  Attrib(const char *loc):
    location(loc)
  {}

  void init() {
    glGenBuffers(1, &vbo); GLERROR
  }

  GLuint id() const {
    return vbo;
  }

  GLuint loc(GLuint program_id) const {
    ASSERT(location != "");
    GLuint lc = glGetAttribLocation(program_id, location.c_str()); GLERROR
    return lc;
  }

  void bind() {
    glBindBuffer(ArrayType, vbo); GLERROR
  }

  template <GLenum Mode>
  void allocate(size_t count, void *host_data) {
    bind();
    glBufferData(ArrayType, count * sizeof(typename a_cast_type<AttribT>::type), host_data, Mode); GLERROR
    Attrib<ArrayType, AttribT>::unbind();
  }

  template <GLenum Mode, typename T>
  void allocate(size_t count, std::vector<T> &&host_data) {
    allocate<Mode>(count, host_data.data());
  }

  decltype(auto) operator[](size_t i) {
    return AttribBufferAccessor(*this, i);
  }

  static void unbind() {
    glBindBuffer(ArrayType, 0); GLERROR
  }

  void clear() {
    glDeleteBuffers(1, &vbo); GLERROR
  }
};

struct VertexArray {
  static GLuint last_vao;
  GLuint vaoId = 0;
  std::vector<gl::GenericShaderAttrib> attribs;

  VertexArray()
  {}
  ~VertexArray()
  {}
  void init() {
    glGenVertexArrays(1, &vaoId); GLERROR
  }
  GLuint id() const {
    return vaoId;
  }
  void bind() {
    /* if(last_vao == vaoId) { */
    /*   return; */
    /* } */
    glBindVertexArray(vaoId); GLERROR
    last_vao = vaoId;
  }
  template <GLenum ArrayType, AttribType AttribT>
  void enable(gl::Attrib<ArrayType, AttribT> &attrib) {
    glEnableVertexAttribArray(attribs.size()); GLERROR
    attribs.push_back(attrib);
  }
  template <GLenum ArrayType, AttribType AttribT>
  void set_access(gl::Attrib<ArrayType, AttribT> a, size_t start=0, size_t stride=0) {
    a.bind();
    using atype = a_cast_type<AttribT>;
    glVertexAttribPointer(start,
      sizeof(typename atype::type) / sizeof(typename atype::vtype),
      gl_scalar_enum<typename atype::vtype>::value,
      GL_FALSE,
      stride,
      nullptr); GLERROR
    Attrib<ArrayType, AttribT>::unbind();
  }
  static void unbind() {
    glBindVertexArray(0); GLERROR
    last_vao = 0;
  }
  void clear() {
    glDeleteVertexArrays(1, &vaoId); GLERROR
  }
};
GLuint VertexArray::last_vao = 0;
}
