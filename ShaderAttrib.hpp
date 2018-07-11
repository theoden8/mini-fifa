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

#define using_sc static constexpr GLenum
template <AttribType A> struct a_cast_type { using type = void; };
template <> struct a_cast_type <AttribType::INTEGER> { using type = GLint; using vtype = GLint; using_sc gltype = GL_INT; };
template <> struct a_cast_type <AttribType::FLOAT> { using type = GLfloat; using vtype  = GLfloat; using_sc gltype = GL_FLOAT; };
template <> struct a_cast_type <AttribType::VEC2> { using type = glm::vec2; using vtype  = GLfloat; using_sc gltype = GL_FLOAT_VEC2; };
template <> struct a_cast_type <AttribType::VEC3> { using type = glm::vec3; using vtype  = GLfloat; using_sc gltype = GL_FLOAT_VEC3; };
template <> struct a_cast_type <AttribType::VEC4> { using type = glm::vec4; using vtype  = GLfloat; using_sc gltype = GL_FLOAT_VEC4; };
template <> struct a_cast_type <AttribType::IVEC2> { using type = glm::ivec2; using vtype  = GLint; using_sc gltype = GL_INT_VEC2; };
template <> struct a_cast_type <AttribType::IVEC3> { using type = glm::ivec3; using vtype  = GLint; using_sc gltype = GL_INT_VEC3; };
template <> struct a_cast_type <AttribType::IVEC4> { using type = glm::ivec4; using vtype  = GLint; using_sc gltype = GL_INT_VEC4; };
template <> struct a_cast_type <AttribType::MAT2> { using type = glm::mat2; using vtype  = GLfloat; using_sc gltype = GL_FLOAT_MAT2; };
template <> struct a_cast_type <AttribType::MAT3> { using type = glm::mat3; using vtype  = GLfloat; using_sc gltype = GL_FLOAT_MAT3; };
template <> struct a_cast_type <AttribType::MAT4> { using type = glm::mat4; using vtype  = GLfloat; using_sc gltype = GL_FLOAT_MAT4; };
#undef using_sc

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
  Attrib(std::string loc):
    location(loc)
  {}

  static void init(GLuint &vbo) {
    glGenBuffers(1, &vbo); GLERROR
  }

  static void init(Attrib<ArrayType, AttribT> &attr) {
    attr.init();
  }

  void init() {
    init(vbo);
  }

  GLuint id() const {
    return vbo;
  }

  GLuint loc(GLuint program_id) const {
    ASSERT(location != "");
    GLuint lc = glGetAttribLocation(program_id, location.c_str()); GLERROR
    return lc;
  }

  static void bind(GLuint vbo) {
    glBindBuffer(ArrayType, vbo); GLERROR
  }

  static void bind(Attrib<ArrayType, AttribT> &attr) {
    attr.bind();
  }

  void bind() {
    bind(vbo);
  }

  bool is_active(GLuint program_id) {
    char name[81];
    GLsizei length;
    GLint size;
    GLenum t;
    glGetActiveAttrib(program_id, vbo, 80, &length, &size, &t, name); GLERROR
    return t == a_cast_type<AttribT>::gltype && location == name;
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
    return AttribBufferAccessor<Attrib<ArrayType, AttribT>>(*this, i);
  }

  static void unbind() {
    glBindBuffer(ArrayType, 0); GLERROR
  }

  static void clear(GLuint &vbo) {
    glDeleteBuffers(1, &vbo); GLERROR
  }

  static void clear(Attrib<ArrayType, AttribT> &attr) {
    attr.clear();
  }

  void clear() {
    clear(vbo);
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

  static void init(GLuint &vao) {
    glGenVertexArrays(1, &vao); GLERROR
  }

  static void init(VertexArray &va) {
    va.init();
  }

  void init() {
    init(vaoId);
  }

  GLuint id() const {
    return vaoId;
  }

  static void bind(GLuint vao) {
    glBindVertexArray(vao); GLERROR
    last_vao = vao;
  }

  static void bind(VertexArray &va) {
    va.bind();
  }

  void bind() {
    /* if(last_vao == vaoId) { */
    /*   return; */
    /* } */
    bind(vaoId);
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

  static void clear(GLuint &vao) {
    glDeleteVertexArrays(1, &vao); GLERROR
  }

  static void clear(VertexArray &va) {
    va.clear();
  }

  void clear() {
    clear(vaoId);
  }
};
GLuint VertexArray::last_vao = 0;
}
