#include "dataconfigreader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <util.h>
#include <functional.h>
#include <dirent.h>

namespace {

const std::string s3d_in = "/input/s3d.in";
const std::string tracer_in = "/input/tracer.in";
const std::string pdf_in = "/input/histogram.in";
const std::string data_out = "/data/";
const std::string tracer_pre = "tracer-";
const std::string pdf_config = "/pdf.config";
const std::string multiblock_config = "/multi_block.config";

std::vector<std::string> getTokensInLine(std::istream &in) {
    std::string line;
    std::getline(in, line);
    std::vector<std::string> tokens;
    while (!line.empty()) {
        std::string::size_type pos = line.find(' ');
        std::string token = line.substr(0, pos);
        if (pos == line.npos)
            pos = line.size() - 1;
        line = line.substr(pos + 1);
        if (!token.empty())
            tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> entryNamesInDirectory(const std::string& directory) {
    std::vector<std::string> entryNames;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            entryNames.push_back(ent->d_name);
        }
        closedir (dir);
    }
    return entryNames;
}

bool isEntryExistInDirectory(
        const std::string& entryName, const std::string& directory) {
    auto entryNames = entryNamesInDirectory(directory);
    return std::any_of(entryNames.begin(), entryNames.end(),
            [entryName](const std::string& element) {
        return entryName == element;
    });
}

template <typename T>
T to(const std::string& str) {
    std::istringstream os(str);
    T output;
    os >> output;
    return output;
}
auto toDouble = to<double>;
auto toInt = to<int>;
auto toFloat = to<float>;

} // unnamed namespace

/**
 * @brief TimeSteps::TimeSteps
 * @param nTimes
 * @param nTimesPerField
 */
TimeSteps::TimeSteps(int nTimes, int nTimesPerField, float freqTracer) {
    int nTimesPerStep = nTimesPerField * freqTracer;
    auto nSteps = nTimes / nTimesPerStep;
    auto interval = 0.5e-8 * nTimesPerStep;
    _timeStepStrs.resize(nSteps);
    for (auto iStep = 0; iStep < nSteps; ++iStep) {
        auto timeStep = interval * (iStep + 1);
        _timeStepStrs[iStep] = yy::sprintf("%.4E", timeStep);
    }
}

double TimeSteps::asDouble(int iStep) const {
    return toDouble(asString(iStep));
}

/**
 * @brief HistConfig::name
 * @return
 */
std::string HistConfig::name() const
{
    std::string ret = vars[0];
    for (unsigned int i = 1; i < vars.size(); ++i) {
        ret += "-" + vars[i];
    }
    return ret;
}

bool HistConfig::load(std::istream &in)
{
    std::string line;
    std::string dimension;
    in >> dimension;
    if (dimension == "dimension") {
        in >> nDim;
    } else {
        nDim = atoi(dimension.c_str());
    }
    std::getline(in, line);
    if (!in) return false;
    vars.resize(nDim);
    rangeMethods.resize(nDim);
    nBins.resize(nDim);
    mins.resize(nDim);
    maxs.resize(nDim);
    /// TODO: actually recognize the comments instead of using getline.
    for (auto iDim = 0; iDim < nDim; ++iDim) {
        in >> vars[iDim] >> nBins[iDim] >> rangeMethods[iDim] >> mins[iDim] >>
                maxs[iDim];
        std::getline(in, line);
    }
    return true;
}

bool operator==(const HistConfig &a, const HistConfig &b) {
    return a.nDim == b.nDim && a.vars == b.vars
            && a.rangeMethods == b.rangeMethods && a.nBins == b.nBins
            && a.mins == b.mins && a.maxs == b.maxs;
}

bool operator!=(const HistConfig &a, const HistConfig &b) {
    return !(a == b);
}


/**
 * @brief S3DDataConfigReader::S3DDataConfigReader
 * @param dir
 */
S3DDataConfigReader::S3DDataConfigReader(const std::string &dir)
  : _dir(dir) {

}

bool S3DDataConfigReader::read() {
    std::ifstream fs3d((_dir + s3d_in).c_str() );
    std::ifstream ftracer((_dir + tracer_in).c_str());
    std::ifstream fhist((_dir + pdf_in).c_str());
    if (!fs3d || !ftracer || !fhist)
        return false;

    std::string line;
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    // _dimVoxels
    _dimVoxels.resize(3);
    fs3d >> _dimVoxels[0]; std::getline(fs3d, line);
    fs3d >> _dimVoxels[1]; std::getline(fs3d, line);
    fs3d >> _dimVoxels[2]; std::getline(fs3d, line);
    // _dimProcs
    _dimProcs.resize(3);
    fs3d >> _dimProcs[0]; std::getline(fs3d, line);
    fs3d >> _dimProcs[1]; std::getline(fs3d, line);
    fs3d >> _dimProcs[2]; std::getline(fs3d, line);

    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    // _nSteps and _interval
    fs3d >> _nTimes;
    std::getline(fs3d, line);
    fs3d >> _nTimesPerField;
    std::getline(fs3d, line);
    std::getline(fs3d, line);

    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);

    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);

    _volMin.resize(3);
    _volMax.resize(3);
    fs3d >> _volMin[0]; _volMin[0] /= 100.f; std::getline(fs3d, line);
    fs3d >> _volMin[1]; _volMin[1] /= 100.f; std::getline(fs3d, line);
    fs3d >> _volMin[2]; _volMin[2] /= 100.f; std::getline(fs3d, line);
    fs3d >> _volMax[0]; _volMax[0] /= 100.f; std::getline(fs3d, line);
    fs3d >> _volMax[1]; _volMax[1] /= 100.f; std::getline(fs3d, line);
    fs3d >> _volMax[2]; _volMax[2] /= 100.f; std::getline(fs3d, line);

    std::getline(ftracer, line);
    std::getline(ftracer, line);
    std::getline(ftracer, line);
    std::getline(ftracer, line);
    ftracer >> _freqTracer;
    std::getline(ftracer, line);

    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    _dimHistsPerDomain.resize(3);
    fhist >> _dimHistsPerDomain[0]
            >> _dimHistsPerDomain[1]
            >> _dimHistsPerDomain[2]; std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    while(fhist) {
        HistConfig histConfig;
        if (histConfig.load(fhist))
            _histConfigs.push_back(histConfig);
    }
    return true;
}

/**
 * @brief PdfDataConfigReader::read
 * @return
 */
bool PdfDataConfigReader::read() {
    std::ifstream fin((_dir + pdf_config).c_str());
    std::vector<std::string> tokens;

//    while (fin) {
//        tokens = getTokensInLine(fin);
//        for (auto token : tokens) {
//            std::cout << token << "    ";
//        }
//        std::cout << std::endl;
//    }

    tokens = getTokensInLine(fin);
    _dimVoxels.resize(3);
    _dimVoxels[0] = atoi(tokens[tokens.size() - 3].c_str());
    _dimVoxels[1] = atoi(tokens[tokens.size() - 2].c_str());
    _dimVoxels[2] = atoi(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _dimProcs.resize(3);
    _dimProcs[0] = atoi(tokens[tokens.size() - 3].c_str());
    _dimProcs[1] = atoi(tokens[tokens.size() - 2].c_str());
    _dimProcs[2] = atoi(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _nTimes = atoi(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _nTimesPerField = atoi(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _volMin.resize(3);
    _volMin[0] = atof(tokens[tokens.size() - 3].c_str());
    _volMin[1] = atof(tokens[tokens.size() - 2].c_str());
    _volMin[2] = atof(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _volMax.resize(3);
    _volMax[0] = atof(tokens[tokens.size() - 3].c_str());
    _volMax[1] = atof(tokens[tokens.size() - 2].c_str());
    _volMax[2] = atof(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _freqTracer = atof(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    _dimHistsPerDomain.resize(3);
    _dimHistsPerDomain[0] = atoi(tokens[tokens.size() - 3].c_str());
    _dimHistsPerDomain[1] = atoi(tokens[tokens.size() - 2].c_str());
    _dimHistsPerDomain[2] = atoi(tokens[tokens.size() - 1].c_str());
    tokens = getTokensInLine(fin);
    assert(tokens[0] == "histograms");
    while (fin) {
        HistConfig histConfig;
        if (histConfig.load(fin)) {
            _histConfigs.push_back(histConfig);
        }

    }
    return true;
}

/**
 * @brief MultiBlockConfigReader::read
 * @return
 */
bool MultiBlockConfigReader::read() {
    PdfDataConfigReader pdfConfigReader(_dir);
    if (!pdfConfigReader.read()) {
        return false;
    }
    _histConfigs = pdfConfigReader.histConfigs();

    std::ifstream fmb((_dir + multiblock_config).c_str());
    std::vector<std::string> tokens;

    tokens = getTokensInLine(fmb);
    auto nBlocks = toInt(tokens[tokens.size() - 1]);
    std::vector<BlockSpec> blockSpecs(nBlocks);
    for (auto iBlock = 0; iBlock < nBlocks; ++iBlock) {
        tokens = getTokensInLine(fmb);
        yy::ivec3 nDomains(
                toInt(tokens[1]), toInt(tokens[2]), toInt(tokens[3]));
        yy::ivec3 nGridPts(
                toInt(tokens[5]), toInt(tokens[6]), toInt(tokens[7]));
        yy::ivec3 lowerCorner(
                toInt(tokens[9]), toInt(tokens[10]), toInt(tokens[11]));
        BoundingBox<float> physicalBoundingBox(
                yy::vec3(toFloat(tokens[13]), toFloat(tokens[14]),
                    toFloat(tokens[15])),
                yy::vec3(toFloat(tokens[17]), toFloat(tokens[18]),
                    toFloat(tokens[19])));
        blockSpecs[iBlock] =
                BlockSpec(nDomains, nGridPts, lowerCorner, physicalBoundingBox);
    }
    _gridConfig = GridConfig(blockSpecs);

    auto entries = entryNamesInDirectory(_dir);
    auto timeStepDirs = yy::fp::filter(entries, [](const std::string& entry) {
        return "pdf-" == entry.substr(0, 4);
    });
    auto timeStepStrs =
            yy::fp::map(timeStepDirs, [](const std::string& timeStepDir) {
        return timeStepDir.substr(4);
    });
    auto sortedTimeStepStrs = yy::fp::sort(timeStepStrs);
    _timeSteps = TimeSteps(sortedTimeStepStrs);

    return true;
}
