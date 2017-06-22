#ifndef GLSHADER_H
#define GLSHADER_H

//glBindAttribLocation
//glBindFragDataLocation
//glBindFragDataLocationIndexed

//glGetActiveAttrib
//glGetActiveUniform
//glGetActiveUniformBlock
//glGetActiveUniformBlockName
//glGetActiveUniformName
//glGetActiveUniformsiv
//glGetAttachedShaders
//glGetFragDataIndex
//glGetFragDataLocation
//glGetProgram
//glGetProgramInfoLog
//glGetShader
//glGetShaderInfoLog
//glGetShaderSource
//glGetUniform
//glGetUniformBlockIndex
//glGetUniformIndices

//glIsProgram
//glIsShader
//glUniformBlockBinding
//glValidateProgram

#include <opengl.h>
#include <string>
#include <iostream>
#include <initializer_list>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <glm/glm.hpp>
#ifdef QT_VERSION
#include <QMatrix4x4>
#endif

namespace yy {
namespace gl {

class shader {
public:
    enum TYPE : GLenum {
        VERTEX_SHADER = GL_VERTEX_SHADER,
        GEOMETRY_SHADER = GL_GEOMETRY_SHADER,
        FRAGMENT_SHADER = GL_FRAGMENT_SHADER
    };

public:
    shader(TYPE type, std::string source) {
        _type = type;
        _glIdx = glCreateShader(type);
        shaderSource(source);
        compileShader();
    }
    shader(const shader& other) : shader(other._type, other._source) {}
    shader& operator=(const shader& other) {
        this->_type = other._type;
        shaderSource(other._source);
        return *this;
    }
    ~shader() {
        glDeleteShader(_glIdx);
    }

public:
    GLuint glIdx() const { return _glIdx; }

private:
    void shaderSource(const GLchar* source, const GLint nBytes) {
        _source.resize(nBytes);
        memcpy(&_source[0], source, nBytes);
        shaderSource();
    }
    void shaderSource(std::string source) {
        _source = source;
        shaderSource();
    }
    void shaderSource() {
        const char* data = _source.data();
        GLint length = _source.size();
        glShaderSource(_glIdx, 1, &data, &length);
    }
    void compileShader() {
        glCompileShader(_glIdx);
        if (!getCompileStatus()) {
            auto infoLog = getShaderInfoLog();
            std::cout << infoLog << std::endl;
            throw infoLog;
        }
    }
    bool getCompileStatus() {
        GLint success = 0;
        glGetShaderiv(_glIdx, GL_COMPILE_STATUS, &success);
        return success == GL_TRUE;
    }
    std::string getShaderInfoLog() {
        GLint logSize = 0;
        glGetShaderiv(_glIdx, GL_INFO_LOG_LENGTH, &logSize);
        std::string infoLog(logSize, '\0');
        glGetShaderInfoLog(_glIdx, logSize, &logSize, &infoLog[0]);
        return infoLog;
    }

private:
    TYPE _type;
    GLuint _glIdx;
    std::string _source;
};

class program {
public:
    program() {
        _glIdx = glCreateProgram();
    }
    program(std::initializer_list<std::shared_ptr<shader>> shaders)
      : program() {
        attachShaders(shaders);
    }
    template <typename... Args>
    program(Args... args) : program() {
        attachShaders(args...);
    }
    program(const program& other) : program() {
        *this = other;
    }
    program& operator=(const program& other) {
        detachAllShaders();
        for (auto keyValue : other._shaderMap) {
            attachShader(keyValue.second);
        }
        linkProgram();
        return *this;
    }
    /// TODO: implement these
    program(std::initializer_list<shader> shaders) = delete;
    ~program() {
        glDeleteProgram(_glIdx);
    }

public:
    template <typename... Args>
    void attachShaders(Args... args) {
        internalAttachShaders(args...);
        linkProgram();
    }
    void attachShaders(std::initializer_list<std::shared_ptr<shader>> shaders) {
        for (auto s : shaders) {
            attachShader(s);
        }
        linkProgram();
    }
    void detachAllShaders() {
        for (auto keyValue : _shaderMap) {
            detachShader(keyValue.first);
        }
    }

public:
    void bind() const {
        /// TODO: throw if no shader is attached.
        _prevProgIdx = getBoundProgramIndex();
        glUseProgram(_glIdx);
    }
    void release() const {
        glUseProgram(_prevProgIdx);
    }
    void bound(const std::function<void()>& functor) const {
        bind();
        functor();
        release();
    }
    template <typename T>
    T bound(const std::function<T()>& functor) const {
        T ret;
        bind();
        ret = functor();
        release();
        return ret;
    }
    bool isBound() const {
        return _glIdx == getBoundProgramIndex();
    }
    static GLuint getBoundProgramIndex() {
        GLint index;
        glGetIntegerv(GL_CURRENT_PROGRAM, &index);
        return index;
    }

public:
    GLint getAttribLocation(const std::string& name) const {
        return bound<GLint>([&, this]() {
            return glGetAttribLocation(_glIdx, name.c_str());
        });
    }

public:
    GLint getUniformLocation(const std::string& name) const {
        return bound<GLint>([&, this]() {
            return glGetUniformLocation(_glIdx, name.c_str());
        });
    }
    template <typename T>
    void setUniform(GLint location, const T& value) {
        bound([&]() {
            internalSetUniform(location, value);
        });
    }
    template <typename T>
    void setUniform(const std::string& name, const T& value) {
        bound([&]() {
            internalSetUniform(name, value);
        });
    }
    void setUniforms() { return; }
    template <typename Loc, typename T, typename... Args>
    void setUniforms(Loc location, T value, Args... args) {
        bound([&]() {
            internalSetUniform(location, value);
            setUniforms(args...);
        });
    }

private:
    template <typename T>
    void internalSetUniform(const std::string& name, const T& value) {
        internalSetUniform(getUniformLocation(name), value);
    }
    void internalSetUniform(GLint location, GLfloat value) {
        uniform1f(location, value);
    }
    void internalSetUniform(GLint location, const glm::vec2& value) {
        uniform2f(location, value.x, value.y);
    }
    void internalSetUniform(GLint location, const glm::vec3& value) {
        uniform3f(location, value.x, value.y, value.z);
    }
    void internalSetUniform(GLint location, const glm::vec4& value) {
        uniform4f(location, value.x, value.y, value.z, value.w);
    }
    void internalSetUniform(GLint location, GLint value) {
        uniform1i(location, value);
    }
    void internalSetUniform(GLint location, const glm::ivec2& value) {
        uniform2i(location, value.x, value.y);
    }
    void internalSetUniform(GLint location, const glm::ivec3& value) {
        uniform3i(location, value.x, value.y, value.z);
    }
    void internalSetUniform(GLint location, const glm::ivec4& value) {
        uniform4i(location, value.x, value.y, value.z, value.w);
    }
    void internalSetUniform(GLint location, const glm::mat4& value) {
        uniformMatrix4fv(location, 1, false, &value[0][0]);
    }
#ifdef QT_VERSION
    void internalSetUniform(GLint location, const QVector3D& value) {
        uniform3f(location, value.x(), value.y(), value.z());
    }
    void internalSetUniform(GLint location, const QMatrix3x3& value) {
        uniformMatrix3fv(location, 1, false, value.data());
    }
    void internalSetUniform(GLint location, const QMatrix4x4& value) {
        uniformMatrix4fv(location, 1, false, value.data());
    }
#endif

    void uniform1f(GLint location, GLfloat v0) {
        glUniform1f(location, v0);
    }
    void uniform2f(GLint location, GLfloat v0, GLfloat v1) {
        glUniform2f(location, v0, v1);
    }
    void uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
        glUniform3f(location, v0, v1, v2);
    }
    void uniform4f(
            GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
        glUniform4f(location, v0, v1, v2, v3);
    }

    void uniform1i(GLint location, GLint v0) {
        glUniform1i(location, v0);
    }
    void uniform2i(GLint location, GLint v0, GLint v1) {
        glUniform2i(location, v0, v1);
    }
    void uniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
        glUniform3i(location, v0, v1, v2);
    }
    void uniform4i(
            GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
        glUniform4i(location, v0, v1, v2, v3);
    }

    void uniform1ui(GLint location, GLuint v0) {
        glUniform1ui(location, v0);
    }
    void uniform2ui(GLint location, GLuint v0, GLuint v1) {
        glUniform2ui(location, v0, v1);
    }
    void uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
        glUniform3ui(location, v0, v1, v2);
    }
    void uniform4ui(
            GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
        glUniform4f(location, v0, v1, v2, v3);
    }

    void uniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose,
            const GLfloat* value) {
        glUniformMatrix3fv(location, count, transpose, value);
    }
    void uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
            const GLfloat* value) {
        glUniformMatrix4fv(location, count, transpose, value);
    }

private:
    void internalAttachShaders() { return; }
    template <typename... Args>
    void internalAttachShaders(
            shader::TYPE type, std::string source, Args... args) {
        attachShader(type, source);
        internalAttachShaders(args...);
    }
    void attachShader(shader::TYPE type, std::string source) {
        auto s = std::make_shared<shader>(type, source);
        attachShader(s);
    }
    void attachShader(std::shared_ptr<class shader> shader) {
        _shaderMap[shader->glIdx()] = shader;
        glAttachShader(_glIdx, shader->glIdx());
    }
    void detachShader(std::shared_ptr<class shader> shader) {
        detachShader(shader->glIdx());
    }
    void detachShader(GLuint shaderIdx) {
        _shaderMap.erase(shaderIdx);
        glDetachShader(_glIdx, shaderIdx);
    }
    void linkProgram() {
        glLinkProgram(_glIdx);
        if (!getLinkStatus()) {
            auto infoLog = getProgramInfoLog();
            std::cout << infoLog << std::endl;
            throw infoLog;
        }
    }
    bool getLinkStatus() {
        GLint isLinked = 0;
        glGetProgramiv(_glIdx, GL_LINK_STATUS, (int *)&isLinked);
        return isLinked == GL_TRUE;
    }
    std::string getProgramInfoLog() {
        GLint maxLength = 0;
        glGetProgramiv(_glIdx, GL_INFO_LOG_LENGTH, &maxLength);
        std::string infoLog(maxLength, '\0');
        glGetProgramInfoLog(_glIdx, maxLength, &maxLength, &infoLog[0]);
        return infoLog;
    }

private:
    GLuint _glIdx;
    mutable GLuint _prevProgIdx;
    std::unordered_map<GLuint, std::shared_ptr<class shader>> _shaderMap;
};

} // namespace gl
} // namespace yy

#endif // GLSHADER_H
