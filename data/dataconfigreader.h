#ifndef DATACONFIGREADER_H
#define DATACONFIGREADER_H

#include <string>
#include <vector>
#include "multiblocktopology.h"

class TimeSteps {
public:
    TimeSteps() = default;
    TimeSteps(std::vector<std::string> timeStepStrs)
          : _timeStepStrs(timeStepStrs) {}
    TimeSteps(int nTimes, int nTimesPerField, float freqTracer);

public:
    int nSteps() const { return _timeStepStrs.size(); }
    std::string asString(int iStep) const { return _timeStepStrs[iStep]; }
    double asDouble(int iStep) const;

private:
    std::vector<std::string> _timeStepStrs;
};

/**
 * @brief The GridConfig struct
 */
class GridConfig {
public:
    enum GridType {GridType_UniformGrid, GridType_MultiBlock};
    GridConfig() = default;
    GridConfig(const std::vector<int>& dimVoxels,
            const std::vector<int>& dimProcs,
            const std::vector<int>& dimHistsPerDomain,
            const std::vector<float>& volMin,
            const std::vector<float>& volMax)
          : _gridType(GridType_UniformGrid),
            _uniformGrid(yy::ivec3(dimVoxels), yy::ivec3(dimProcs),
                yy::ivec3(dimHistsPerDomain),
                BoundingBox<float>(yy::vec3(volMin), yy::vec3(volMax))) {}
    GridConfig(std::vector<BlockSpec> blockSpecs)
          : _gridType(GridType_MultiBlock), _multiBlocks(blockSpecs) {}

public:
    GridType gridType() const { return _gridType; }
    UniformGridTopology uniformGrid() const { return _uniformGrid; }
    MultiBlockTopology multiBlocks() const { return _multiBlocks; }

public:
    BoundingBox<float> physicalBoundingBox() const {
        return _uniformGrid.physicalBoundingBox();
    }

// backward compatibility
public:
    yy::ivec3 dimHists() const {
        return _uniformGrid.dimHistsPerDomain() * _uniformGrid.dimProcs();
    }
    yy::ivec3 dimProcs() const { return _uniformGrid.dimProcs(); }
    yy::ivec3 dimHistsPerDomain() const {
        return _uniformGrid.dimHistsPerDomain();
    }
    yy::ivec3 dimVoxels() const { return _uniformGrid.dimVoxels(); }

private:
    GridType _gridType;
    UniformGridTopology _uniformGrid;
    MultiBlockTopology _multiBlocks;
};

/**
 * @brief The HistConfig struct
 */
struct HistConfig {
    int nDim;
    std::vector<std::string> vars;
    std::vector<std::string> rangeMethods;
    std::vector<std::string> nBins;
    std::vector<double> mins;
    std::vector<double> maxs;

    std::string name() const;
    bool load(std::istream& in);
};
bool operator==(const HistConfig& a, const HistConfig& b);
bool operator!=(const HistConfig& a, const HistConfig& b);

/**
 * @brief The DataConfigReader class
 */
class DataConfigReader {
public:
    virtual ~DataConfigReader() {}

public:
    virtual bool read() = 0;

public:
    virtual GridConfig gridConfig() const = 0;
    virtual TimeSteps timeSteps() const = 0;
    virtual std::vector<HistConfig> histConfigs() const = 0;
};

/**
 * @brief The S3DDataConfigReader class
 */
class S3DDataConfigReader : public DataConfigReader {
public:
    S3DDataConfigReader() = delete;
    S3DDataConfigReader(const std::string& dir);

public:
    bool read();

public:
    GridConfig gridConfig() const {
        return GridConfig(
                _dimVoxels, _dimProcs, _dimHistsPerDomain, _volMin, _volMax);
    }
    TimeSteps timeSteps() const {
        return TimeSteps(_nTimes, _nTimesPerField, _freqTracer);
    }
    std::vector<int> dimVoxels() const { return _dimVoxels; }
    std::vector<int> dimProcs() const { return _dimProcs; }
    int nTimes() const { return _nTimes; }
    int nTimesPerField() const { return _nTimesPerField; }
    std::vector<float> volMin() const { return _volMin; }
    std::vector<float> volMax() const { return _volMax; }
    float freqTracer() const { return _freqTracer; }
    std::vector<int> dimHistsPerDomain() const { return _dimHistsPerDomain; }
    std::vector<HistConfig> histConfigs() const { return _histConfigs; }

private:
    std::string _dir;
    std::vector<int> _dimVoxels, _dimProcs, _dimHistsPerDomain;
    int _nTimes, _nTimesPerField;
    std::vector<float> _volMin, _volMax;
    float _freqTracer;
    std::vector<HistConfig> _histConfigs;
};

/**
 * @brief The PdfDataConfigReader class
 */
class PdfDataConfigReader : public DataConfigReader {
public:
    PdfDataConfigReader() = delete;
    PdfDataConfigReader(const std::string& dir) : _dir(dir) {}

public:
    bool read();

public:
    GridConfig gridConfig() const {
        return GridConfig(
                _dimVoxels, _dimProcs, _dimHistsPerDomain, _volMin, _volMax);
    }
    TimeSteps timeSteps() const {
        return TimeSteps(_nTimes, _nTimesPerField, _freqTracer);
    }
    std::vector<int> dimVoxels() const { return _dimVoxels; }
    std::vector<int> dimProcs() const { return _dimProcs; }
    int nTimes() const { return _nTimes; }
    int nTimesPerField() const { return _nTimesPerField; }
    std::vector<float> volMin() const { return _volMin; }
    std::vector<float> volMax() const { return _volMax; }
    float freqTracer() const { return _freqTracer; }
    std::vector<int> dimHistsPerDomain() const { return _dimHistsPerDomain; }
    std::vector<HistConfig> histConfigs() const { return _histConfigs; }

private:
    std::string _dir;
    std::vector<int> _dimVoxels, _dimProcs, _dimHistsPerDomain;
    int _nTimes, _nTimesPerField;
    std::vector<float> _volMin, _volMax;
    float _freqTracer;
    std::vector<HistConfig> _histConfigs;
};

/**
 * @brief The MultiBlockConfigReader class
 */
class MultiBlockConfigReader : public DataConfigReader {
public:
    MultiBlockConfigReader() = delete;
    MultiBlockConfigReader(const std::string& dir) : _dir(dir) {}

public:
    bool read();

public:
    GridConfig gridConfig() const { return _gridConfig; }
    TimeSteps timeSteps() const { return _timeSteps; }
    std::vector<HistConfig> histConfigs() const { return _histConfigs; }

private:
    std::string _dir;
    GridConfig _gridConfig;
    TimeSteps _timeSteps;
    std::vector<HistConfig> _histConfigs;
};

#endif // DATACONFIGREADER_H
