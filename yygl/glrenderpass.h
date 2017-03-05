#ifndef GLRENDERPASS_H
#define GLRENDERPASS_H

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <yygl/glvertexarray.h>
#include <yygl/glshader.h>
#include <yygl/gltexture.h>
#include <yygl/glvector.h>
#include <yygl/glerror.h>

namespace yy {
namespace gl {

class texture_attrib {
public:
    texture_attrib()
      : _uniformLocation(0), _textureLocation(0)
      , _target(texture::DEFAULT_TARGET) {}
    texture_attrib(std::shared_ptr<class texture> tex, GLint uniformLocation,
            GLint textureLocation, texture::TARGET target)
      : _texture(tex), _uniformLocation(uniformLocation)
      , _textureLocation(textureLocation), _target(target) {}

public:
    std::shared_ptr<class texture> texture() const { return _texture; }
    GLint uniformLocation() const { return _uniformLocation; }
    GLint textureLocation() const { return _textureLocation; }
    texture::TARGET target() const { return _target; }

private:
    std::shared_ptr<class texture> _texture;
    GLint _uniformLocation;
    GLint _textureLocation;
    texture::TARGET _target;
};

class render_pass {
public:
    enum DRAW_MODE : GLenum {
        POINTS = GL_POINTS,
        LINE_STRIP = GL_LINE_STRIP,
        LINE_LOOP = GL_LINE_LOOP,
        LINES = GL_LINES,
        LINE_STRIP_ADJACENCY = GL_LINE_STRIP_ADJACENCY,
        LINES_ADJACENCY = GL_LINES_ADJACENCY,
        TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        TRIANGLE_FAN = GL_TRIANGLE_FAN,
        TRIANGLES = GL_TRIANGLES,
        TRIANGLE_STRIP_ADJACENCY = GL_TRIANGLE_STRIP_ADJACENCY,
        TRIANGLES_ADJACENCY = GL_TRIANGLES_ADJACENCY
    };
    enum DRAW_INDEX_TYPE : GLenum {
        UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
        UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
        UNSIGNED_INT = GL_UNSIGNED_INT
    };
    void drawArrays() {
        for (auto keyValue : _uniformFunctorMap)
            keyValue.second();
        _program->bound([this]() {
            _vao.bound([this]() {
                _textureMap.bound(_program.get(), [this]() {
                    drawArrays(_drawMode, _firstVertIdx, _nVerts);
                });
            });
        });
    }
    void drawElements() {
        for (auto keyValue : _uniformFunctorMap)
            keyValue.second();
        _program->bound([this]() {
            _vao.bound([this]() {
                _textureMap.bound(_program.get(), [this]() {
                    drawElements(_drawMode, _nVerts, _iboType,
                            reinterpret_cast<const GLvoid*>(_firstVertIdx));
                });
            });
        });
    }
    static void drawArrays(DRAW_MODE mode, GLint firstVertIdx, GLsizei nVerts) {
        error::flush();
        glDrawArrays(mode, firstVertIdx, nVerts);
        error::throwIfError();
    }
    static void drawElements(DRAW_MODE mode, GLsizei count,
            DRAW_INDEX_TYPE type, const GLvoid* indices) {
        glDrawElements(mode, count, type, indices);
    }

public:
    /// TODO: at the moment program has be set first because of the
    /// getUniformLocation calls. It would be nice to decouple it.
    void setProgram(std::shared_ptr<class program> program) {
        _program = program;
    }
    template <typename... Args>
    void setProgram(Args... args) {
        _program = std::make_shared<program>(args...);
    }
    void setTexture(std::shared_ptr<texture> texture,
            const std::string& uniformName, GLint textureLocation,
            texture::TARGET target) {
        auto uniformLocation = _program->getUniformLocation(uniformName);
        _textureMap[uniformLocation] =
                texture_attrib(
                    texture, uniformLocation, textureLocation, target);
    }
    void setVBO(const std::string& attribName, std::shared_ptr<buffer> buf,
            GLint nComponents, GLenum dataType,
            GLboolean normalized = GL_FALSE, GLsizei stride = 0,
            GLsizei offset = 0) {
        auto attribLocation = _program->getAttribLocation(attribName);
        _vboMap[attribLocation] = buf;
        _vao.vertexAttribPointer(attribLocation, buf.get(), nComponents,
                dataType, normalized, stride, offset);
    }
    template <typename T>
    void setVBO(const std::string& attribName, const vector<T>& vec,
            GLboolean normalized = GL_FALSE, GLsizei stride = 0,
            GLsizei offset = 0) {
        setVBO(attribName, vec.data(), type_map<T>::nComponents(),
                type_map<T>::dataType(), normalized, stride, offset);
    }
    template <typename T>
    void setIBO(const vector<T>& vec) {
        _ibo = vec.data();
        _vao.elementArray(_ibo.get());
        _iboType = DRAW_INDEX_TYPE(type_map<T>::dataType());
    }
    void setDrawMode(DRAW_MODE mode) {
        _drawMode = mode;
    }
    void setFirstVertexIndex(GLint firstVertIdx) {
        _firstVertIdx = firstVertIdx;
    }
    void setVertexCount(GLint nVerts) {
        _nVerts = nVerts;
    }
    template <typename T>
    void setUniform(GLint location, const T& value) {
        /// TODO: consider using std::mem_fn instead of std::bind.
        _uniformFunctorMap[location] =
                std::bind(
                    static_cast<void(program::*)(GLint, const T&)>(
                        &program::setUniform),
                    _program.get(), location, value);
    }
    template <typename T>
    void setUniform(const std::string& name, const T& value) {
        setUniform(_program->getUniformLocation(name), value);
    }
    void setUniforms() { return; }
    template <typename U, typename T, typename... Args>
    void setUniforms(U name, const T& value, Args... args) {
        setUniform(name, value);
        setUniforms(args...);
    }

private:
    class texture_map {
    public:
        texture_attrib& operator[](GLint key) {
            return _theMap[key];
        }
        const texture_attrib& operator[](GLint key) const {
            return _theMap.at(key);
        }

    public:
        void bind(class program* program) const {
            for (auto keyValue : _theMap) {
                const texture_attrib& textureAttrib = keyValue.second;
                program->setUniform(
                        textureAttrib.uniformLocation(),
                        textureAttrib.textureLocation());
                textureAttrib.texture()->activeBindByLocation(
                        textureAttrib.textureLocation(),
                        textureAttrib.target());
            }
        }
        void release() const {
            for (auto keyValue : _theMap) {
                const texture_attrib& textureAttrib = keyValue.second;
                textureAttrib.texture()->activeReleaseByLocation(
                        textureAttrib.textureLocation(),
                        textureAttrib.target());
            }
        }
        void bound(class program* program,
                const std::function<void()>& functor) const {
            bind(program);
            functor();
            release();
        }

    private:
        std::unordered_map<GLint, texture_attrib> _theMap;
    };

private:
    vertex_array _vao;
    std::shared_ptr<program> _program;
    texture_map _textureMap;
    std::unordered_map<GLint, std::shared_ptr<buffer>> _vboMap;
    std::shared_ptr<buffer> _ibo;
    DRAW_MODE _drawMode = POINTS;
    DRAW_INDEX_TYPE _iboType;
    GLint _firstVertIdx = 0;
    GLint _nVerts = 0;
    std::unordered_map<GLint, std::function<void()>> _uniformFunctorMap;
};

} // namespace gl
} // namespace yy

#endif // GLRENDERPASS_H
