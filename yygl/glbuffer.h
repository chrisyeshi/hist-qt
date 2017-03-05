#ifndef __GL_BUFFER_H__
#define __GL_BUFFER_H__

#include <vector>
#include <map>
#include <functional>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace yy {
namespace gl {

class buffer {
public:
    enum TARGET : GLenum {
        ARRAY_BUFFER = GL_ARRAY_BUFFER,
        COPY_READ_BUFFER = GL_COPY_READ_BUFFER,
        COPY_WRITE_BUFFER = GL_COPY_WRITE_BUFFER,
        ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER,
        PIXEL_PACK_BUFFER = GL_PIXEL_PACK_BUFFER,
        PIXEL_UNPACK_BUFFER = GL_PIXEL_UNPACK_BUFFER,
        TEXTURE_BUFFER = GL_TEXTURE_BUFFER,
        TRANSFORM_FEEDBACK_BUFFER = GL_TRANSFORM_FEEDBACK_BUFFER,
        UNIFORM_BUFFER = GL_UNIFORM_BUFFER
    };
    static const TARGET DEFAULT_TARGET = ARRAY_BUFFER;
    enum TARGET_BINDING : GLenum {
        ARRAY_BUFFER_BINDING = GL_ARRAY_BUFFER_BINDING,
        ELEMENT_ARRAY_BUFFER_BINDING = GL_ELEMENT_ARRAY_BUFFER_BINDING
    };
    static const std::map<TARGET, TARGET_BINDING>& target_to_binding() {
        static std::map<TARGET, TARGET_BINDING> theMap = {
            { ARRAY_BUFFER, ARRAY_BUFFER_BINDING },
            { ELEMENT_ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER_BINDING }
        };
        return theMap;
    }
    enum USAGE : GLenum {
        STREAM_DRAW = GL_STREAM_DRAW,
        STREAM_READ = GL_STREAM_READ,
        STREAM_COPY = GL_STREAM_COPY,
        STATIC_DRAW = GL_STATIC_DRAW,
        STATIC_READ = GL_STATIC_READ,
        STATIC_COPY = GL_STATIC_COPY,
        DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
        DYNAMIC_READ = GL_DYNAMIC_READ,
        DYNAMIC_COPY = GL_DYNAMIC_COPY
    };
    static const USAGE DEFAULT_USAGE = STATIC_DRAW;
    enum ACCESS : GLenum {
        READ_ONLY = GL_READ_ONLY,
        WRITE_ONLY = GL_WRITE_ONLY,
        READ_WRITE = GL_READ_WRITE
    };
    static const ACCESS DEFAULT_ACCESS = READ_WRITE;
    enum PNAME : GLenum {
        BUFFER_ACCESS = GL_BUFFER_ACCESS,
        BUFFER_ACCESS_FLAGS = GL_BUFFER_ACCESS_FLAGS,
        // BUFFER_IMMUTABLE_STORAGE = GL_BUFFER_IMMUTABLE_STORAGE,
        BUFFER_MAP_OFFSET = GL_BUFFER_MAP_OFFSET,
        BUFFER_SIZE = GL_BUFFER_SIZE,
        // BUFFER_STORAGE_FLAGS = GL_BUFFER_STORAGE_FLAGS,
        BUFFER_USAGE = GL_BUFFER_USAGE
    };
    enum PNAME64 : GLenum {
        BUFFER_MAPPED = GL_BUFFER_MAPPED,
        BUFFER_MAP_LENGTH = GL_BUFFER_MAP_LENGTH
    };

public:
    buffer()
      : mappedPtr(nullptr) {
        glGenBuffers(1, &glIdx);
    }
    buffer(size_t nBytes, USAGE usage = DEFAULT_USAGE) : buffer() {
        bufferData(nBytes, NULL, usage);
    }
    /// TODO: add move construtor.
    buffer(const buffer& other) : buffer(other.size()) {
        *this = other;
    }
    buffer& operator=(const buffer& other) {
        bufferData(other.nBytes(), NULL);
        other.bound(COPY_READ_BUFFER, [&]() {
            this->bound(COPY_WRITE_BUFFER, [&]() {
                glCopyBufferSubData(COPY_READ_BUFFER, COPY_WRITE_BUFFER,
                        0, 0, other.size());
            }); // this->bound
        }); // other.bound
        return *this;
    }
    ~buffer() {
        glDeleteBuffers(1, &glIdx);
    }

public:
    size_t nBytes() const { return size(); }
    size_t size(TARGET target = DEFAULT_TARGET) const {
        GLint nBytes = 0;
        bound(target, [&]() {
            getBufferParameteriv(target, BUFFER_SIZE, &nBytes);
        });
        return nBytes;
    }

public:
#ifdef GSL
    template <typename U>
    buffer& operator(const gsl::span<const U>& data) {
        bufferData(data);
        return *this;
    }
    template <typename U>
    void bufferData(
            const gsl::span<const U>& data, GLenum usage = DEFAULT_USAGE) {
        bufferData(DEFAULT_TARGET, data, usage);
    }
    template <typename U>
    void bufferData(TARGET target, const gsl::span<const U>& data,
            GLenum usage = DEFAULT_USAGE) {
        bufferData(target, data.extent(0),
                reinterpret_cast<GLvoid*>(data.data()), usage);
    }
#endif
    /// TODO: operator= initializer list
    template <typename U>
    buffer& operator=(const std::vector<U>& data) {
        bufferData(data);
        return *this;
    }
    template <typename U>
    void bufferData(
            const std::vector<U>& data, GLenum usage = DEFAULT_USAGE) {
        bufferData(DEFAULT_TARGET, data, usage);
    }
    template <typename U>
    void bufferData(TARGET target, const std::vector<U>& data,
            GLenum usage = DEFAULT_USAGE) {
        bufferData(target, data.size() * sizeof(U),
                reinterpret_cast<const GLvoid*>(data.data()), usage);
    }
    void bufferData(size_t nBytes, const GLvoid* data,
            GLenum usage = DEFAULT_USAGE) {
        bufferData(DEFAULT_TARGET, nBytes, data, usage);
    }
    void bufferData(TARGET target, size_t nBytes, const GLvoid* data,
            GLenum usage = DEFAULT_USAGE) {
        bound(target, [&]() {
            glBufferData(target, nBytes, data, usage);
        });
    }
    void bufferSubData(GLintptr offset, GLsizeiptr nBytes, const GLvoid* data) {
        bufferSubData(DEFAULT_TARGET, offset, nBytes, data);
    }
    void bufferSubData(TARGET target, GLintptr offset, GLsizeiptr nBytes,
            const GLvoid* data) {
        bound(target, [&]() {
            glBufferSubData(target, offset, nBytes, data);
        });
    }

public:
    template <typename U = GLbyte>
    operator std::vector<U>() const {
        return getBufferData<U>();
    }
    template <typename U = GLbyte>
    std::vector<U> getBufferData(TARGET target = DEFAULT_TARGET) const {
        return getBufferData<U>(target, 0, nBytes());
    }
    template <typename U = GLbyte>
    std::vector<U> getBufferData(GLintptr offset, GLsizeiptr nBytes) const {
        return getBufferData<U>(DEFAULT_TARGET, offset, nBytes);
    }
    template <typename U = GLbyte>
    std::vector<U> getBufferData(
            TARGET target, GLintptr offset, GLsizeiptr nBytes) const {
        std::vector<U> data(nBytes / sizeof(U));
        getBufferData(
                target, offset, nBytes, reinterpret_cast<GLvoid*>(data.data()));
        return data;
    }
    void getBufferData(GLvoid* data) const {
        getBufferData(DEFAULT_TARGET, data);
    }
    void getBufferData(TARGET target, GLvoid* data) const {
        getBufferData(target, 0, nBytes(), data);
    }
    void getBufferData(GLintptr offset, GLsizeiptr nBytes, GLvoid* data) const {
        getBufferData(DEFAULT_TARGET, offset, nBytes, data);
    }
    void getBufferData(TARGET target, GLintptr offset, GLsizeiptr nBytes,
            GLvoid* data) const {
        bound(target, [&]() {
            glGetBufferSubData(target, offset, nBytes, data);
        });
    }

public:
    /// TODO: test the binding functions...
    void bind(TARGET target = DEFAULT_TARGET) const {
        if (0 < target_to_binding().count(target)) {
            auto binding = target_to_binding().at(target);
            prevBufIdx = getBoundBufferIndex(binding);
        } else {
            prevBufIdx = 0;
        }
        glBindBuffer(target, glIdx);
    }
    void release(TARGET target = DEFAULT_TARGET) const {
        glBindBuffer(target, prevBufIdx);
        prevBufIdx = 0;
    }
    void bound(const std::function<void()>& functor) const {
        bound(DEFAULT_TARGET, functor);
    }
    void bound(TARGET target, const std::function<void()>& functor) const {
        bind(target);
        functor();
        release();
    }
    bool isBound(TARGET_BINDING targetBinding) const {
        return glIdx == getBoundBufferIndex(targetBinding);
    }

public:
    GLbyte* map(
            TARGET target = DEFAULT_TARGET, ACCESS access = DEFAULT_ACCESS) {
        bind(target);
        mappedPtr = reinterpret_cast<GLbyte*>(glMapBuffer(target, access));
        release(target);
        return mappedPtr;
    }
    void unmap(TARGET target = DEFAULT_TARGET) {
        if (!isMapped())
            return;
        bind(target);
        glUnmapBuffer(target);
        mappedPtr = nullptr;
        release(target);
    }
    bool isMapped() {
        return nullptr != mappedPtr;
    }
    void mapped(const std::function<void()>& functor) {
        mapped(DEFAULT_TARGET, functor);
    }
    void mapped(TARGET target, const std::function<void()>& functor) {
        mapped(target, DEFAULT_ACCESS, functor);
    }
    void mapped(TARGET target, ACCESS access,
            const std::function<void()>& functor) {
        map(target, access);
        functor();
        unmap(target);
    }
    GLbyte* mappedData() { return mappedPtr; }

protected:
    static GLuint getBoundBufferIndex(TARGET_BINDING targetBinding) {
        GLint index;
        glGetIntegerv(targetBinding, &index);
        return index;
    }
    static void getBufferParameteriv(TARGET target, PNAME pname, GLint* data) {
        glGetBufferParameteriv(target, pname, data);
    }
    static void getBufferParameteriv(
            TARGET target, PNAME64 pname, GLint64* data) {
        glGetBufferParameteri64v(target, pname, data);
    }

private:
    GLuint glIdx;
    mutable GLuint prevBufIdx;
    GLbyte* mappedPtr;
};

} // namespace gl
} // namespace yy

#endif // __GL_BUFFER_H__
