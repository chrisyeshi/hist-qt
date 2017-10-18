#ifndef GLTEXTURE_H
#define GLTEXTURE_H

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <map>
#include <vector>
#include <initializer_list>
#include "gltype.h"

namespace yy {
namespace gl {

class texture {
public:
    enum TARGET : GLenum {
        TEXTURE_1D = GL_TEXTURE_1D,
        TEXTURE_2D = GL_TEXTURE_2D,
        TEXTURE_3D = GL_TEXTURE_3D,
        TEXTURE_1D_ARRAY = GL_TEXTURE_1D_ARRAY,
        TEXTURE_2D_ARRAY = GL_TEXTURE_2D_ARRAY,
        TEXTURE_RECTANGLE = GL_TEXTURE_RECTANGLE,
        TEXTURE_CUBE_MAP = GL_TEXTURE_CUBE_MAP,
        TEXTURE_BUFFER = GL_TEXTURE_BUFFER,
        TEXTURE_2D_MULTISAMPLE = GL_TEXTURE_2D_MULTISAMPLE,
        TEXTURE_2D_MULTISAMPLE_ARRAY = GL_TEXTURE_2D_MULTISAMPLE_ARRAY
    };
    static const TARGET DEFAULT_TARGET = TEXTURE_2D;
    enum TARGET_BINDING : GLenum {
        TEXTURE_BINDING_1D = GL_TEXTURE_BINDING_1D,
        TEXTURE_BINDING_1D_ARRAY = GL_TEXTURE_BINDING_1D_ARRAY,
        TEXTURE_BINDING_2D = GL_TEXTURE_BINDING_2D,
        TEXTURE_BINDING_2D_ARRAY = GL_TEXTURE_BINDING_2D_ARRAY,
        TEXTURE_BINDING_2D_MULTISAMPLE = GL_TEXTURE_BINDING_2D_MULTISAMPLE,
        TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY =
                GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY,
        TEXTURE_BINDING_3D = GL_TEXTURE_BINDING_3D,
        TEXTURE_BINDING_BUFFER = GL_TEXTURE_BINDING_BUFFER,
        TEXTURE_BINDING_CUBE_MAP = GL_TEXTURE_BINDING_CUBE_MAP,
        TEXTURE_BINDING_RECTANGLE = GL_TEXTURE_BINDING_RECTANGLE
    };
    static const std::map<TARGET, TARGET_BINDING>& target_to_binding() {
        static std::map<TARGET, TARGET_BINDING> theMap = {
            { TEXTURE_1D, TEXTURE_BINDING_1D },
            { TEXTURE_1D_ARRAY, TEXTURE_BINDING_1D_ARRAY },
            { TEXTURE_2D, TEXTURE_BINDING_2D },
            { TEXTURE_2D_ARRAY, TEXTURE_BINDING_2D_ARRAY },
            { TEXTURE_2D_MULTISAMPLE, TEXTURE_BINDING_2D_MULTISAMPLE },
            { TEXTURE_2D_MULTISAMPLE_ARRAY,
                    TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY },
            { TEXTURE_3D, TEXTURE_BINDING_3D },
            { TEXTURE_BUFFER, TEXTURE_BINDING_BUFFER },
            { TEXTURE_CUBE_MAP, TEXTURE_BINDING_CUBE_MAP },
            { TEXTURE_RECTANGLE, TEXTURE_BINDING_RECTANGLE }
        };
        return theMap;
    }
    enum PARAMETER : GLenum {
        TEXTURE_BASE_LEVEL = GL_TEXTURE_BASE_LEVEL,
        TEXTURE_BORDER_COLOR = GL_TEXTURE_BORDER_COLOR,
        TEXTURE_COMPARE_FUNC = GL_TEXTURE_COMPARE_FUNC,
        TEXTURE_COMPARE_MODE = GL_TEXTURE_COMPARE_MODE,
        TEXTURE_LOD_BIAS = GL_TEXTURE_LOD_BIAS,
        TEXTURE_MIN_FILTER = GL_TEXTURE_MIN_FILTER,
        TEXTURE_MAG_FILTER = GL_TEXTURE_MAG_FILTER,
        TEXTURE_MIN_LOD = GL_TEXTURE_MIN_LOD,
        TEXTURE_MAX_LOD = GL_TEXTURE_MAX_LOD,
        TEXTURE_MAX_LEVEL = GL_TEXTURE_MAX_LEVEL,
        TEXTURE_SWIZZLE_R = GL_TEXTURE_SWIZZLE_R,
        TEXTURE_SWIZZLE_G = GL_TEXTURE_SWIZZLE_G,
        TEXTURE_SWIZZLE_B = GL_TEXTURE_SWIZZLE_B,
        TEXTURE_SWIZZLE_A = GL_TEXTURE_SWIZZLE_A,
        TEXTURE_SWIZZLE_RGBA = GL_TEXTURE_SWIZZLE_RGBA,
        TEXTURE_WRAP_S = GL_TEXTURE_WRAP_S,
        TEXTURE_WRAP_T = GL_TEXTURE_WRAP_T,
        TEXTURE_WRAP_R = GL_TEXTURE_WRAP_R
    };
    enum MIN_FILTER : GLint {
        MIN_NEAREST = GL_NEAREST,
        MIN_LINEAR = GL_LINEAR,
        MIN_NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
        MIN_LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
        MIN_NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
        MIN_LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
    };
    enum MAG_FILTER : GLint {
        MAG_NEAREST = GL_NEAREST,
        MAG_LINAER = GL_LINEAR
    };
    enum INTERNAL_FORMAT : GLenum {
        INTERNAL_RGBA32F = GL_RGBA32F,
        INTERNAL_RGBA32I = GL_RGBA32I,
        INTERNAL_RGBA32UI = GL_RGBA32UI,
        INTERNAL_RGBA16 = GL_RGBA16,
        INTERNAL_RGBA16F = GL_RGBA16F,
        INTERNAL_RGBA16I = GL_RGBA16I,
        INTERNAL_RGBA16UI = GL_RGBA16UI,
        INTERNAL_RGBA8 = GL_RGBA8,
        INTERNAL_RGBA8UI = GL_RGBA8UI,
        INTERNAL_SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
        INTERNAL_RGB10_A2 = GL_RGB10_A2,
        INTERNAL_RGB10_A2UI = GL_RGB10_A2UI,
        INTERNAL_R11F_G11F_B10F = GL_R11F_G11F_B10F,
        INTERNAL_RG32F = GL_RG32F,
        INTERNAL_RG32I = GL_RG32I,
        INTERNAL_RG32UI = GL_RG32UI,
        INTERNAL_RG16 = GL_RG16,
        INTERNAL_RG16F = GL_RG16F,
        INTERNAL_RGB16I = GL_RGB16I,
        INTERNAL_RGB16UI = GL_RGB16UI,
        INTERNAL_RG8 = GL_RG8,
        INTERNAL_RG8I = GL_RG8I,
        INTERNAL_RG8UI = GL_RG8UI,
        INTERNAL_R32F = GL_R32F,
        INTERNAL_R32I = GL_R32I,
        INTERNAL_R32UI = GL_R32UI,
        INTERNAL_R16F = GL_R16F,
        INTERNAL_R16I = GL_R16I,
        INTERNAL_R16UI = GL_R16UI,
        INTERNAL_R8 = GL_R8,
        INTERNAL_R8I = GL_R8I,
        INTERNAL_R8UI = GL_R8UI,
        INTERNAL_RGBA16_SNORM = GL_RGBA16_SNORM,
        INTERNAL_RGBA8_SNORM = GL_RGBA8_SNORM,
        INTERNAL_RGB32F = GL_RGB32F,
        INTERNAL_RGB32I = GL_RGB32I,
        INTERNAL_RGB32UI = GL_RGB32UI,
        INTERNAL_RGB16_SNORM = GL_RGB16_SNORM,
        INTERNAL_RGB16F = GL_RGB16F,
        INTERNAL_RGB16 = GL_RGB16,
        INTERNAL_RGB8_SNORM = GL_RGB8_SNORM,
        INTERNAL_RGB8 = GL_RGB8,
        INTERNAL_RGB8I = GL_RGB8I,
        INTERNAL_RGB8UI = GL_RGB8UI,
        INTERNAL_SRGB8 = GL_SRGB8,
        INTERNAL_RGB9_E5 = GL_RGB9_E5,
        INTERNAL_RG16_SNORM = GL_RG16_SNORM,
        INTERNAL_RG8_SNORM = GL_RG8_SNORM,
        INTERNAL_COMPRESSED_RG_RGTC2 = GL_COMPRESSED_RG_RGTC2,
        INTERNAL_COMPRESSED_SIGNED_RG_RGTC2 = GL_COMPRESSED_SIGNED_RG_RGTC2,
        INTERNAL_R16_SNORM = GL_R16_SNORM,
        INTERNAL_R8_SNORM = GL_R8_SNORM,
        INTERNAL_COMPRESSED_RED_RGTC1 = GL_COMPRESSED_RED_RGTC1,
        INTERNAL_COMPRESSED_SIGNED_RED_RGTC1 = GL_COMPRESSED_SIGNED_RED_RGTC1,
        INTERNAL_DEPTH_COMPONENT32F = GL_DEPTH_COMPONENT32F,
        INTERNAL_DEPTH_COMPONENT24 = GL_DEPTH_COMPONENT24,
        INTERNAL_DEPTH_COMPONENT16 = GL_DEPTH_COMPONENT16,
        INTERNAL_DEPTH32F_STENCIL8 = GL_DEPTH32F_STENCIL8,
        INTERNAL_DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8
    };
    enum FORMAT : GLenum {
        FORMAT_RED = GL_RED,
        FORMAT_RG = GL_RG,
        FORMAT_RGB = GL_RGB,
        FORMAT_BGR = GL_BGR,
        FORMAT_RGBA = GL_RGBA,
        FORMAT_BGRA = GL_BGRA
    };
    enum DATA_TYPE : GLenum {
        UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
        BYTE = GL_BYTE,
        UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
        SHORT = GL_SHORT,
        UNSIGNED_INT = GL_UNSIGNED_INT,
        INT = GL_INT,
        FLOAT = GL_FLOAT,
        UNSIGNED_BYTE_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
        UNSIGNED_BYTE_2_3_3_REV = GL_UNSIGNED_BYTE_2_3_3_REV,
        UNSIGNED_SHORT_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
        UNSIGNED_SHORT_5_6_5_REV = GL_UNSIGNED_SHORT_5_6_5_REV,
        UNSIGNED_SHORT_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
        UNSIGNED_SHORT_4_4_4_4_REV = GL_UNSIGNED_SHORT_4_4_4_4_REV,
        UNSIGNED_SHORT_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
        UNSIGNED_SHORT_1_5_5_5_REV = GL_UNSIGNED_SHORT_1_5_5_5_REV,
        UNSIGNED_INT_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
        UNSIGNED_INT_8_8_8_8_REV = GL_UNSIGNED_INT_8_8_8_8_REV,
        UNSIGNED_INT_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
        UNSIGNED_INT_2_10_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV
    };
    enum WRAP_MODE : GLint {
        CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
        CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
        MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
        REPEAT = GL_REPEAT
    };

public:
    texture() {
        glGenTextures(1, &_glIdx);
    }
    /// TODO: implement these
    texture(const texture& other) = delete;
    texture& operator=(const texture& other) = delete;
    ~texture() {
        glDeleteTextures(1, &_glIdx);
    }

public:
    template <typename T>
    void texImage1D(TARGET target, INTERNAL_FORMAT internalFormat,
            FORMAT format, std::initializer_list<T> initData) {
        std::vector<T> data(initData);
        texImage1D(target, internalFormat, data.size(), format,
                DATA_TYPE(type_map<T>::dataType()), data.data());
    }
    void texImage1D(TARGET target, INTERNAL_FORMAT internalFormat,
            GLsizei width, FORMAT format, DATA_TYPE type, const GLvoid* data) {
        texImage1D(target, 0, internalFormat, width, 0, format, type, data);
    }
    void texImage1D(TARGET target, GLint level, INTERNAL_FORMAT internalFormat,
            GLsizei width, GLint border, FORMAT format, DATA_TYPE type,
            const GLvoid* data) {
        bound(target, [&]() {
            glTexImage1D(target, level, internalFormat, width, border, format,
                    type, data);
        });
    }
    void texImage2D(TARGET target, GLint level, INTERNAL_FORMAT internalFormat,
            GLsizei width, GLsizei height, GLint border, FORMAT format,
            DATA_TYPE type, const GLvoid* data) {
        bound(target, [&]() {
            glTexImage2D(target, level, internalFormat, width, height, border,
                    format, type, data);
        });
    }
    void texImage2D(TARGET target, INTERNAL_FORMAT internalFormat,
            GLsizei width, GLsizei height, FORMAT format, DATA_TYPE type,
            const GLvoid* data) {
        texImage2D(target, 0, internalFormat, width, height, 0, format, type,
                data);
    }
    void texImage2D(INTERNAL_FORMAT internalFormat, GLsizei width,
            GLsizei height, FORMAT format, DATA_TYPE type, const GLvoid* data) {
        texImage2D(
                TEXTURE_2D, internalFormat, width, height, format, type, data);
    }
    void texImage3D(TARGET target, GLint level, INTERNAL_FORMAT internalFormat,
            GLsizei width, GLsizei height, GLsizei depth, GLint border,
            FORMAT format, DATA_TYPE type, const GLvoid* data) {
        bound(target, [&]() {
            glTexImage3D(target, level, internalFormat, width, height, depth,
                    border, format, type, data);
        });
    }
    void texImage3D(TARGET target, INTERNAL_FORMAT internalFormat,
            GLsizei width, GLsizei height, GLsizei depth, FORMAT format,
            DATA_TYPE type, const GLvoid* data) {
        texImage3D(target, 0, internalFormat, width, height, depth, 0, format,
                type, data);
    }
    void texImage3D(INTERNAL_FORMAT internalFormat, GLsizei width,
            GLsizei height, GLsizei depth, FORMAT format, DATA_TYPE type,
            const GLvoid* data) {
        texImage3D(TEXTURE_3D, internalFormat, width, height, depth, format,
            type, data);
    }

public:
    void setWrapMode(TARGET target, WRAP_MODE wrapMode) {
        setWrapModeS(target, wrapMode);
        setWrapModeT(target, wrapMode);
        setWrapModeR(target, wrapMode);
    }
    void setWrapModeS(TARGET target, WRAP_MODE wrapMode) {
        texParameteri(target, TEXTURE_WRAP_S, wrapMode);
    }
    void setWrapModeT(TARGET target, WRAP_MODE wrapMode) {
        texParameteri(target, TEXTURE_WRAP_T, wrapMode);
    }
    void setWrapModeR(TARGET target, WRAP_MODE wrapMode) {
        texParameteri(target, TEXTURE_WRAP_R, wrapMode);
    }
    void setTextureMinMagFilter(
            TARGET target, MIN_FILTER minFilter, MAG_FILTER magFilter) {
        setTextureMinFilter(target, minFilter);
        setTextureMagFilter(target, magFilter);
    }
    void setTextureMinFilter(TARGET target, MIN_FILTER filter) {
        texParameteri(target, TEXTURE_MIN_FILTER, filter);
    }
    void setTextureMagFilter(TARGET target, MAG_FILTER filter) {
        texParameteri(target, TEXTURE_MAG_FILTER, filter);
    }
    void texParameteri(TARGET target, PARAMETER parameter, GLint value) {
        bound(target, [&]() {
            glTexParameteri(target, parameter, value);
        });
    }

public:
    void activeBind(GLenum texture, TARGET target) const {
        activeTexture(texture);
        bind(target);
    }
    void activeBindByLocation(GLint location, TARGET target) const {
        activeTextureByLocation(location);
        bind(target);
    }
    void activeRelease(GLenum texture, TARGET target) const {
        activeTexture(texture);
        release(target);
    }
    void activeReleaseByLocation(GLint location, TARGET target) const {
        activeTextureByLocation(location);
        release(target);
    }
    void activeBound(GLenum texture, TARGET target,
            const std::function<void()>& functor) const {
        activeBind(texture, target);
        functor();
        activeRelease(texture, target);
    }
    void activeBoundByLocation(GLint location, TARGET target,
            const std::function<void()>& functor) const {
        activeBindByLocation(location, target);
        functor();
        activeRelease(location, target);
    }
    void bind(TARGET target) const {
        _prevTexIdx = getBoundTextureIndex(target);
        glBindTexture(target, _glIdx);
    }
    void release(TARGET target) const {
        glBindTexture(target, _prevTexIdx);
    }
    void bound(TARGET target, const std::function<void()>& functor) const {
        bind(target);
        functor();
        release(target);
    }
    bool isBound(TARGET target) const {
        return _glIdx == getBoundTextureIndex(target);
    }
    static GLuint getBoundTextureIndex(TARGET target) {
        return getBoundTextureIndex(target_to_binding().at(target));
    }
    static GLuint getBoundTextureIndex(TARGET_BINDING targetBinding) {
        GLint index;
        glGetIntegerv(targetBinding, &index);
        return index;
    }
    static void activeTexture(GLenum texture) {
        glActiveTexture(texture);
    }
    static void activeTextureByLocation(GLint location) {
        activeTexture(GL_TEXTURE0 + location);
    }

private:
    GLuint _glIdx;
    mutable GLuint _prevTexIdx;
};

} // namespace gl
} // namespace yy

#endif // GLTEXTURE_H
