#ifndef __FORTRANREADER_H__
#define __FORTRANREADER_H__

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <cstring>

#define YESSWAP
class ByteOrder
{
public:
    template <typename T>
    static T swap(T value)
    {
#ifndef YESSWAP
        return value;
#else
        T out;
        std::reverse_copy(reinterpret_cast<char*>(&value), reinterpret_cast<char*>(&value) + sizeof(T), reinterpret_cast<char*>(&out));
        return out;
#endif
    }
};

//
//
//
// Buffer
//
//
//

class Buffer
{
public:
    Buffer() {}
    Buffer(const std::vector<unsigned char>& array) { buffer = array; }
    ~Buffer() {}

public:
    int32_t toInt32() const { return toSingle<int32_t>(); }
    int64_t toInt64() const { return toSingle<int64_t>(); }
    float toFloat() const { return toSingle<float>(); }
    double toDouble() const { return toSingle<double>(); }
    std::vector<char> toCharArray() const { return toArray<char>(); }
    std::vector<int32_t> toInt32Array() const { return toArray<int32_t>(); }
    std::vector<int64_t> toInt64Array() const { return toArray<int64_t>(); }
    std::vector<float> toFloatArray() const { return toArray<float>(); }
    std::vector<double> toDoubleArray() const { return toArray<double>(); }

    template <class T>
    T toSingle() const
    {
        assert(buffer.size() == sizeof(T));
        T single;
        std::memcpy(&single, buffer.data(), buffer.size());
        return ByteOrder::swap(single);
    }

    template <class T>
    std::vector<T> toArray() const
    {
        std::vector<T> array(buffer.size() / sizeof(T));
        std::memcpy(array.data(), buffer.data(), buffer.size());
#ifdef YESSWAP
        for (unsigned int i = 0; i < array.size(); ++i)
            array[i] = ByteOrder::swap(array[i]);
#endif
        return array;
    }

private:
    std::vector<unsigned char> buffer;
};

//
//
//
// FortranReader
//
//
//

class FortranReader
{
public:
    FortranReader(const std::string& filename)
    {
        fin.open(filename.c_str(), std::ios::in | std::ios::binary);
    }
    ~FortranReader() {}

public:
    int currReadPos() {
        return fin.tellg();
    }
    void setReadPosFromBeg(int readPos) {
        fin.seekg(readPos, std::ios::beg);
    }

public:
    int32_t readInt32() { return readRecord().toInt32(); }
    int64_t readInt64() { return readRecord().toInt64(); }
    float readFloat() { return readRecord().toFloat(); }
    double readDouble() { return readRecord().toDouble(); }
    std::vector<char> readCharArray() { return readRecord().toCharArray(); }
    std::vector<int32_t> readInt32Array() { return readRecord().toInt32Array(); }
    std::vector<int64_t> readInt64Array() { return readRecord().toInt64Array(); }
    std::vector<float> readFloatArray() { return readRecord().toFloatArray(); }
    std::vector<double> readDoubleArray() { return readRecord().toDoubleArray(); }

    template <class T>
    std::vector<T> readArray() { return readRecord().toArray<T>(); }

    template <typename T>
    std::vector<T> readSubArray(int offset, int nElements) {
        return readSubRecord(offset * sizeof(T), nElements * sizeof(T))
                .toArray<T>();
    }

    void ignoreRecord()
    {
        int32_t length, endlen;
        fin.read(reinterpret_cast<char*>(&length), sizeof(length));
        length = ByteOrder::swap(length);
        fin.seekg(length, fin.cur);
        fin.read(reinterpret_cast<char*>(&endlen), sizeof(endlen));
        endlen = ByteOrder::swap(endlen);
        // std::cout << length << " : " << endlen << std::endl;
        assert(length == endlen);
    }

protected:
    Buffer readRecord()
    {
        int32_t length, endlen;
        fin.read(reinterpret_cast<char*>(&length), sizeof(length));
        length = ByteOrder::swap(length);
        std::vector<unsigned char> buffer(length);
        fin.read(reinterpret_cast<char*>(buffer.data()), length);
        fin.read(reinterpret_cast<char*>(&endlen), sizeof(endlen));
        endlen = ByteOrder::swap(endlen);
        // std::cout << length << " : " << endlen << std::endl;
        assert(length == endlen);
        return Buffer(buffer);
    }

    Buffer readSubRecord(int offset, int nBytes) {
        int32_t length, endlen;
        fin.read(reinterpret_cast<char*>(&length), sizeof(length));
        length = ByteOrder::swap(length);
        assert(offset + nBytes <= length);
        std::vector<unsigned char> buffer(nBytes);
        fin.seekg(offset, std::ios::cur);
        fin.read(reinterpret_cast<char*>(buffer.data()), nBytes);
        fin.seekg(length - offset - nBytes, std::ios::cur);
        fin.read(reinterpret_cast<char*>(&endlen), sizeof(endlen));
        endlen = ByteOrder::swap(endlen);
        assert(length == endlen);
        return Buffer(buffer);
    }

private:
    std::ifstream fin;
};

#endif // __FORTRANRADER_H__
