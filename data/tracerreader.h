#ifndef TRACERREADER_H
#define TRACERREADER_H

#include <algorithm>
#include <unordered_map>
#include <memory>
#include <cstdio>
#include <fstream>
#include "fortranreader.h"
#include "Extent.h"
#include <glm/vec3.hpp>


class TracerConfig {
public:
    TracerConfig(std::string dir, const std::vector<int>& dimDomains,
            const std::vector<int>& dimHistsPerDomain, const std::vector<int>& dimVoxels)
      : m_dir(dir), m_dimDomains(dimDomains), m_dimHistsPerDomain(dimHistsPerDomain), m_dimVoxels(dimVoxels) {}
    const std::string& dir() const { return m_dir; }
    Extent dimDomains() const { return Extent(m_dimDomains); }
    Extent dimHists() const { return Extent(
            m_dimDomains[0] * m_dimHistsPerDomain[0],
            m_dimDomains[1] * m_dimHistsPerDomain[1],
            m_dimDomains[2] * m_dimHistsPerDomain[2]); }
    Extent dimHistsPerDomain() const { return Extent(m_dimHistsPerDomain); }
    Extent dimVoxels() const { return Extent(m_dimVoxels); }

private:
    std::string m_dir;
    std::vector<int> m_dimDomains;
    std::vector<int> m_dimHistsPerDomain;
    std::vector<int> m_dimVoxels;
};



struct Region
{
    Region(double xmin, double ymin, double zmin, double xmax, double ymax, double zmax)
      : xmin(xmin), ymin(ymin), zmin(zmin), xmax(xmax), ymax(ymax), zmax(zmax) {}

    double xmin, ymin, zmin, xmax, ymax, zmax;

    bool contains(double x, double y, double z) const
    {
        if (x < xmin || x >= xmax) return false;
        if (y < ymin || y >= ymax) return false;
        if (z < zmin || z >= zmax) return false;
        return true;
    }
};



struct Particle
{
    int64_t ssn;
    glm::dvec3 loc, xloc, dloc, vel;
    double rho, temp, mixfrac, scaldis;

    bool inRegion(const Region& domain) const {
        return domain.contains(loc.x, loc.y, loc.z);
    }
};

std::ostream& operator<<(std::ostream& os, const Particle& p);



class TracerFileReader
{
public:
    TracerFileReader(const std::string& filename) : reader(filename) {}
    ~TracerFileReader() {}

public:
    int currReadPos() { return reader.currReadPos(); }
    void setReadPosFromBeg(int readPos) { reader.setReadPosFromBeg(readPos); }

public:
    void ignoreRecord()  { reader.ignoreRecord(); }
    int32_t readInt32()  { return reader.readInt32(); }
    int64_t readInt64()  { return reader.readInt64(); }
    float   readFloat()  { return reader.readFloat(); }
    double  readDouble() { return reader.readDouble(); }
    std::vector<int32_t> readInt32Array() { return readArray<int32_t>(); }
    std::vector<int64_t> readInt64Array() { return readArray<int64_t>(); }
    std::vector<float>   readFloatArray() { return readArray<float>(); }
    std::vector<double>  readDoubleArray() { return readArray<double>(); }
    void ignoreArray()
    {
        reader.ignoreRecord();
        reader.ignoreRecord();
        reader.ignoreRecord();
    }

    template <class T>
    std::vector<T> readArray()
    {
        // TODO: currently ignores variable name string
        std::vector<char> varfile = reader.readCharArray();
        std::string varStr(varfile.begin(), varfile.end());
        double ref = reader.readDouble();
//        std::cout << varStr << ": " << ref << std::endl;
		// printf("%s: %f\n", varfile.data(), ref);
        std::vector<T> array = reader.readArray<T>();
        for (unsigned int i = 0; i < array.size(); ++i)
            array[i] *= ref;
        return array;
    }

    template <class T>
    bool readArrayIfNameIs(const std::string& name, std::vector<T>& out) {
        std::vector<char> varfile = reader.readCharArray();
        std::string varStr(varfile.begin(), varfile.end());
//        std::cout << varStr << std::endl;
        if (0 != varStr.compare(0, name.size(), name)) {
            reader.ignoreRecord();
            reader.ignoreRecord();
            return false;
        }
        double ref = reader.readDouble();
        out = reader.readArray<T>();
        for (auto& ele : out)
            ele *= ref;
        return true;
    }

    template <typename T>
    bool readSubArrayIfNameIs(const std::string& name, int offset,
            int nElements, std::vector<T>& out) {
        std::vector<char> varfile = reader.readCharArray();
        std::string varStr(varfile.begin(), varfile.end());
//        std::cout << varStr << std::endl;
        if (0 != varStr.compare(0, name.size(), name)) {
            reader.ignoreRecord();
            reader.ignoreRecord();
            return false;
        }
        double ref = reader.readDouble();
        out = reader.readSubArray<T>(offset, nElements);
        for (auto& ele : out)
            ele *= ref;
        return true;
    }

private:
    FortranReader reader;
};



class TracerReader
{
public:
    static std::shared_ptr<TracerReader> create(const TracerConfig& config);
    virtual ~TracerReader() {}

public:
    virtual std::vector<Particle> read(
            const std::vector<int>& selectedHistFlatIds) const = 0;
};



class NullTracerReader : public TracerReader {
public:
    virtual std::vector<Particle> read(
            const std::vector<int>& selectedHistFlatIds) const override {
        return std::vector<Particle>();
    }
};



class OrigTracerReader : public TracerReader
{
public:
    OrigTracerReader(const TracerConfig& config) : m_config(config) {}
    virtual std::vector<Particle> read(
            const std::vector<int>& selectedHistFlatIds) const;

protected:
    typedef std::unordered_map<int, std::vector<std::vector<int>>> DomainMap;
    virtual std::vector<Particle> readYColumnDomains(
            int yColumnFlatId, const DomainMap& dMap) const;

protected:
    TracerConfig m_config;
};

class SortedOrigTracerReader : public OrigTracerReader
{
public:
    SortedOrigTracerReader(const TracerConfig& config)
      : OrigTracerReader(config) {}

protected:
    virtual std::vector<Particle> readYColumnDomains(
            int yColumnFlatId, const DomainMap& dMap) const override;
};

class DomainTracerReader : public TracerReader
{
public:
    DomainTracerReader(const TracerConfig& config) : m_config(config) {}
    virtual std::vector<Particle> read(
            const std::vector<int> &selectedHistFlatIds) const;

protected:
    virtual std::vector<Particle> readLocal(
            const std::string& dir, int dId, int hId) const = 0;

protected:
    TracerConfig m_config;
};


class ManyFilesTracerReader : public DomainTracerReader
{
public:
    ManyFilesTracerReader(const TracerConfig& config) : DomainTracerReader(config) {}

protected:
    virtual std::vector<Particle> readLocal(
            const std::string& dir, int dId, int hId) const;
};

class DomainSortedTracerReader : public DomainTracerReader
{
public:
    DomainSortedTracerReader(const TracerConfig& config) : DomainTracerReader(config) {}

protected:
    virtual std::vector<Particle> readLocal(
            const std::string& dir, int dId, int hId) const;
};

#endif // TRACERREADER_H
