#ifndef GLERROR_H
#define GLERROR_H

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#endif
#include <iostream>

namespace yy {
namespace gl {

class error {
public:
    static void flush() {
        while(GL_NO_ERROR != getError()) {}
    }
    static void throwIfError() {
        auto err = getError();
        if (GL_NO_ERROR != err) {
            switch (err) {
            case GL_INVALID_ENUM:
                std::cout << "GL_INVALID_ENUM" << std::endl;
                assert(false);
                throw "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:
                std::cout << "GL_INVALID_VALUE" << std::endl;
                assert(false);
                throw "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:
                std::cout << "GL_INVALID_OPERATION" << std::endl;
                assert(false);
                throw "GL_INVALID_OPERATION";
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << std::endl;
                assert(false);
                throw "GL_INVALID_FRAMEBUFFER_OPERATION";
            case GL_OUT_OF_MEMORY:
                std::cout << "GL_OUT_OF_MEMORY" << std::endl;
                assert(false);
                throw "GL_OUT_OF_MEMORY";
            }
            std::cout << "Unknown Error: " + std::to_string(err) << std::endl;
            assert(false);
            throw "Unknown Error: " + std::to_string(err);
        }
    }
    static GLenum getError() {
        return glGetError();
    }
};

} // namespace gl
} // namespace yy

#endif // GLERROR_H
