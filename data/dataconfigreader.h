#ifndef DATACONFIGREADER_H
#define DATACONFIGREADER_H

#include <string>
#include <vector>

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

/**
 * @brief The DataConfigReader class
 */
class DataConfigReader {
public:
    virtual ~DataConfigReader() {}

public:
    virtual bool read() = 0;

public:
    virtual std::vector<int> dimVoxels() const = 0;
    virtual std::vector<int> dimProcs() const = 0;
    virtual int nTimes() const = 0;
    virtual int nTimesPerField() const = 0;
    virtual std::vector<float> volMin() const = 0;
    virtual std::vector<float> volMax() const = 0;
    virtual float freqTracer() const = 0;
    virtual std::vector<int> dimHistsPerDomain() const = 0;
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

class PdfDataConfigReader : public DataConfigReader {
public:
    PdfDataConfigReader() = delete;
    PdfDataConfigReader(const std::string& dir) : _dir(dir) {}

public:
    bool read();

public:
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
    std::vector<std::string> getTokensInLine(std::istream& in);

private:
    std::string _dir;
    std::vector<int> _dimVoxels, _dimProcs, _dimHistsPerDomain;
    int _nTimes, _nTimesPerField;
    std::vector<float> _volMin, _volMax;
    float _freqTracer;
    std::vector<HistConfig> _histConfigs;
};

#endif // DATACONFIGREADER_H
