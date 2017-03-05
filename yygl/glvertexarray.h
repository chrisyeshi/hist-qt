#ifndef __GL_VERTEX_ARRAY_H__
#define __GL_VERTEX_ARRAY_H__

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include "glbuffer.h"
#include "glvector.h"
#include "gltype.h"
#include "glerror.h"

namespace yy {
namespace gl {

class vertex_attrib {
public:
    vertex_attrib() : _buffer(nullptr) {}
    vertex_attrib(const buffer* buf, GLint nComponents, GLenum dataType,
            GLboolean normalized = GL_FALSE, GLsizei stride = 0,
            GLsizei offset = 0)
      : _buffer(buf), _nComponents(nComponents), _dataType(dataType)
      , _normalized(normalized), _stride(stride), _offset(offset) {}
    template <typename T>
    vertex_attrib(const vector<T>& data, GLboolean normalized = GL_FALSE,
            GLsizei stride = 0, GLsizei offset = 0)
      : _buffer(&data.data()), _nComponents(type_map<T>::nComponents())
      , _dataType(type_map<T>::dataType()), _normalized(normalized)
      , _stride(stride), _offset(offset) {}

public:
    const class buffer* buffer() const { return _buffer; }
    GLint nComponents() const { return _nComponents; }
    GLenum dataType() const { return _dataType; }
    GLboolean normalized() const { return _normalized; }
    GLsizei stride() const { return _stride; }
    GLsizei offset() const { return _offset; }

private:
    const class buffer* _buffer;
    GLint _nComponents;
    GLenum _dataType;
    GLboolean _normalized;
    GLsizei _stride;
    GLsizei _offset;
};

class vertex_array {
public:
    vertex_array()
      : _glIdx(0), areVertexAttribsBound(true), indexBuffer(nullptr) {}
    vertex_array(const vertex_array& other)
      : _glIdx(0)
      , areVertexAttribsBound(false)
      , vertexAttribs(other.vertexAttribs)
      , indexBuffer(other.indexBuffer) {}
    ~vertex_array() {
        if (0 != _glIdx)
            glDeleteVertexArrays(1, &_glIdx);
    }

public:
    vertex_attrib& operator[](int i) {
        areVertexAttribsBound = false;
        return vertexAttribs[i];
    }
    void vertexAttribPointer(int index, const buffer* buf, GLint nComponents,
            GLenum dataType, GLboolean normalized = GL_FALSE,
            GLsizei stride = 0, GLsizei offset = 0) {
        areVertexAttribsBound = false;
        vertexAttribs[index] = vertex_attrib(
                buf, nComponents, dataType, normalized, stride, offset);
    }
    void elementArray(const buffer* buf) {
        areVertexAttribsBound = false;
        indexBuffer = buf;
    }
    void bindVertexAttribs() {
        for (auto kv : vertexAttribs) {
            GLuint index = kv.first;
            auto vertexAttrib = kv.second;
            enableVertexAttribArray(index);
            vertexAttrib.buffer()->bound([&]() {
                vertexAttribPointer(
                        index, vertexAttrib.nComponents(),
                        vertexAttrib.dataType(), vertexAttrib.normalized(),
                        vertexAttrib.stride(),
                        reinterpret_cast<GLvoid*>(vertexAttrib.offset()));
            });
        }
        if (indexBuffer)
            indexBuffer->bind(buffer::ELEMENT_ARRAY_BUFFER);
        areVertexAttribsBound = true;
    }

public:
    void bind() {
        prevVAIdx = getBoundVertexArrayIndex();
        glBindVertexArray(glIdx());
        if (!areVertexAttribsBound) {
            bindVertexAttribs();
        }
    }
    void release() {
        glBindVertexArray(prevVAIdx);
    }
    bool isBound() {
        return glIdx() == getBoundVertexArrayIndex();
    }
    void bound(const std::function<void()>& functor) {
        bind();
        functor();
        release();
    }

private:
    GLuint getBoundVertexArrayIndex() {
        GLint vaIdx = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaIdx);
        return vaIdx;
    }
    void enableVertexAttribArray(GLuint index) {
        glEnableVertexAttribArray(index);
    }
    void disableVertexAttribArray(GLuint index) {
        glDisableVertexAttribArray(index);
    }
    void vertexAttribPointer(
            GLuint index, GLint size, GLenum type, GLboolean normalized,
            GLsizei stride, const GLvoid* pointer) {
        glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    }

private:
    GLuint glIdx() {
        if (0 == _glIdx) {
            glGenVertexArrays(1, &_glIdx);
        }
        return _glIdx;
    }

private:
    GLuint _glIdx;
    mutable GLuint prevVAIdx;
    mutable bool areVertexAttribsBound;
    std::unordered_map<GLuint, vertex_attrib> vertexAttribs;
    const buffer* indexBuffer;
};

} // namespace gl
} // namespace yy

#endif // __GL_VERTEX_ARRAY_H__
