#ifndef __GL_VECTOR_H__
#define __GL_VECTOR_H__

#include <vector>
#include <cassert>
#include "glbuffer.h"

namespace yy {
namespace gl {

template <typename T>
class vector {
public:
    /// TODO: add allocators to the constructors.
    vector() : synchronized(true) {}
    vector(size_t count, const T& value = T())
      : hostVector(count, value), synchronized(false) {
        syncFromHostToDevice();
    }
    template <class InputIt>
    vector(InputIt first, InputIt last)
      : hostVector(first, last), synchronized(false) {
    }
    vector(std::initializer_list<T> init)
      : hostVector(init), synchronized(false) {
    }
    vector& operator=(std::initializer_list<T> init) {
        synchronized = false;
        hostVector = init;
        return *this;
    }
    ~vector() {}

public:
    void assign(size_t count, T value) {
        hostVector.assign(count, value);
        synchronized = false;
    }
    template <class InputIt>
    void assign(InputIt first, InputIt last) {
        hostVector.assign(first, last);
        synchronized = false;
    }
    void assign(std::initializer_list<T> ilist) {
        hostVector.assign(ilist);
        synchronized = false;
    }

public:
    const T& at(size_t i) const { return hostVector[i]; }
    T& at(size_t i) {
        synchronized = false;
        return hostVector[i];
    }
    const T& operator[](int i) const { return hostVector[i]; }
    T& operator[](int i) {
        synchronized = false;
        return hostVector[i];
    }
    const T& front() const { return hostVector.front(); }
    T& front() {
        synchronized = false;
        return hostVector.front();
    }
    const T& back() const { return hostVector.back(); }
    T& back() {
        synchronized = false;
        return hostVector.back();
    }

public:
    /// TODO: implement a new iteraotr class to also modify the mapped memory.
    typename std::vector<T>::iterator begin() {
        synchronized = false;
        return hostVector.begin();
    }
    typename std::vector<T>::const_iterator begin() const {
        return hostVector.begin();
    }
    typename std::vector<T>::iterator end() {
        synchronized = false;
        return hostVector.end();
    }
    typename std::vector<T>::const_iterator end() const {
        return hostVector.end();
    }
    std::shared_ptr<buffer> data() const {
        if (!synchronized)
            syncFromHostToDevice();
        return deviceBuffer;
    }

public:
    void bind(buffer::TARGET target = buffer::DEFAULT_TARGET) {
        if (!synchronized)
            syncFromHostToDevice();
        deviceBuffer->bind(target);
    }
    void release() { deviceBuffer->release(); }
    void bound(const std::function<void()>& functor) {
        bound(buffer::DEFAULT_TARGET, functor);
    }
    void bound(buffer::TARGET target, const std::function<void()>& functor) {
        if (!synchronized)
            syncFromHostToDevice();
        deviceBuffer->bound(target, functor);
    }

private:
    void syncFromHostToDevice() const {
        if (!deviceBuffer) {
            deviceBuffer = std::make_shared<buffer>();
        }
        *deviceBuffer = hostVector;
        synchronized = true;
    }
//    void syncFromDeviceToHost() {
//        hostVector = deviceBuffer;
//        synchronized = true;
//    }

private:
    mutable std::shared_ptr<buffer> deviceBuffer;
    std::vector<T> hostVector;
    mutable bool synchronized;
};

} // namespace gl
} // namespace yy

#endif // __GL_VECTOR_H__
