#ifndef __GL_TYPE_H__
#define __GL_TYPE_H__

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <glm/glm.hpp>

namespace yy {
namespace gl {

template <typename T>
struct type_map {
    static inline GLint nComponents() { assert(false); return -1; }
    static inline GLenum dataType() { assert(false); return GL_INVALID_ENUM; }
};
// GLbyte
template <> inline GLint type_map<GLbyte>::nComponents() { return 1; }
template <> inline GLenum type_map<GLbyte>::dataType() { return GL_BYTE; }
//// GLubyte
//template <> const GLint type_map<GLubyte>::nComponents = 1;
//template <> const GLenum type_map<GLubyte>::dataType = GL_UNSIGNED_BYTE;
//// GLshort
//template <> const GLint type_map<GLshort>::nComponents = 1;
//template <> const GLenum type_map<GLshort>::dataType = GL_SHORT;
//// GLushort
//template <> const GLint type_map<GLushort>::nComponents = 1;
//template <> const GLenum type_map<GLushort>::dataType = GL_UNSIGNED_SHORT;
// GLint
template <> inline GLint type_map<GLint>::nComponents() { return 1; }
template <> inline GLenum type_map<GLint>::dataType() { return GL_INT; }
// GLuint
template <> inline GLint type_map<GLuint>::nComponents() { return 1; }
template <> inline GLenum type_map<GLuint>::dataType() {
    return GL_UNSIGNED_INT;
}
//// GLfixed
//// template <> const GLint type_map<GLfixed>::nComponents = 1;
//// template <> const GLenum type_map<GLfixed>::dataType = GL_FIXED;
//// GLhalf
//// template <> const GLint type_map<GLhalf>::nComponents = 1;
//// template <> const GLenum type_map<GLhalf>::dataType = GL_HALF_FLOAT;
// GLfloat
template <> inline GLint type_map<GLfloat>::nComponents() { return 1; }
template <> inline GLenum type_map<GLfloat>::dataType() { return GL_FLOAT; }
// GLdouble
template <> inline GLint type_map<GLdouble>::nComponents() { return 1; }
template <> inline GLenum type_map<GLdouble>::dataType() { return GL_DOUBLE; }

#define GLM
#ifdef GLM

// vec2
template <> inline GLint type_map<glm::vec2>::nComponents() { return 2; }
template <> inline GLenum type_map<glm::vec2>::dataType() { return GL_FLOAT; }
// vec3
template <> inline GLint type_map<glm::vec3>::nComponents() { return 3; }
template <> inline GLenum type_map<glm::vec3>::dataType() { return GL_FLOAT; }

//// dvec2
//template <> const GLint type_map<dvec2>::nComponents = 2;
//template <> const GLenum type_map<dvec2>::dataType = GL_DOUBLE;
// dvec3
template <> inline GLint type_map<glm::dvec3>::nComponents() { return 3; }
template <> inline GLenum type_map<glm::dvec3>::dataType() { return GL_DOUBLE; }
//// dvec4
//template <> const GLint type_map<dvec4>::nComponents = 4;
//template <> const GLenum type_map<dvec4>::dataType = GL_DOUBLE;
//// ivec2
//template <> const GLint type_map<ivec2>::nComponents = 2;
//template <> const GLenum type_map<ivec2>::dataType = GL_INT;
//// ivec3
//template <> const GLint type_map<ivec3>::nComponents = 3;
//template <> const GLenum type_map<ivec3>::dataType = GL_INT;
//// ivec4
//template <> const GLint type_map<ivec4>::nComponents = 4;
//template <> const GLenum type_map<ivec4>::dataType = GL_INT;
//// uvec2
//template <> const GLint type_map<uvec2>::nComponents = 2;
//template <> const GLenum type_map<uvec2>::dataType = GL_UNSIGNED_INT;
//// uvec3
//template <> const GLint type_map<uvec3>::nComponents = 3;
//template <> const GLenum type_map<uvec3>::dataType = GL_UNSIGNED_INT;
//// uvec4
//template <> const GLint type_map<uvec4>::nComponents = 4;
//template <> const GLenum type_map<uvec4>::dataType = GL_UNSIGNED_INT;
//// vec2
//template <> const GLint type_map<vec2>::nComponents = 2;
//template <> const GLenum type_map<vec2>::dataType = GL_FLOAT;
//// vec3
//template <> const GLint type_map<vec3>::nComponents = 3;
//template <> const GLenum type_map<vec3>::dataType = GL_FLOAT;
//// vec4
//template <> const GLint type_map<vec4>::nComponents = 4;
//template <> const GLenum type_map<vec4>::dataType = GL_FLOAT;

#endif

} // namespace gl
} // namespace yy

#endif // __GL_TYPE_H__
