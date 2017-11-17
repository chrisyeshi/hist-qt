#ifndef HISTREADER_H
#define HISTREADER_H

#include <memory>
#include <vector>
#include <string>

struct HistHelper;
class Hist;
class HistDomain;
class HistFacadeDomain;

class HistMetaReader {
public:
    bool readFrom(std::istream& fin);

public:
    int ndim = -1, ngridx, ngridy, ngridz, nhistx, nhisty, nhistz;
    std::vector<double> logbases;
};

class HistReaderPacked {
public:
    void readFrom(std::istream& fin, int ndim, std::vector<double> logbases,
            std::vector<std::string> vars);

public:
    std::shared_ptr<Hist> hist;
};

/**
 * @brief The HistDomainReaderManyFiles class
 */
class HistDomainReaderManyFiles {
public:
    HistDomainReaderManyFiles(
            const std::string& dir, const std::string& name,
            const std::string& iProcStr, const std::vector<std::string>& vars)
      : m_dir(dir), m_name(name), m_iProcStr(iProcStr), m_vars(vars) {}
    virtual ~HistDomainReaderManyFiles() {}

public:
    virtual void read(
            HistHelper& histHelper, std::vector<std::shared_ptr<Hist> >& hists);

private:
    HistHelper readHelper(std::vector<int> &nbins, std::vector<double> &mins,
            std::vector<double> &maxs, std::vector<double> &logBases);
    std::vector<int> readOffsets();
    std::vector<int> readBinIds();
    std::vector<double> readValues();

private:
    std::string m_dir, m_name, m_iProcStr;
    std::vector<std::string> m_vars;
};

/**
 * @brief The HistDomainReaderPacked class
 */
class HistDomainReaderPacked {
public:
    HistDomainReaderPacked(
            const std::string& dir, const std::string& name,
            const std::string& iProcStr, const std::vector<std::string>& vars)
      : m_dir(dir), m_name(name), m_iProcStr(iProcStr), m_vars(vars) {}
    virtual ~HistDomainReaderPacked() {}

public:
    virtual void read(
            HistHelper& histHelper, std::vector<std::shared_ptr<Hist> >& hists);

private:
    std::string m_dir, m_name, m_iProcStr;
    std::vector<std::string> m_vars;
};

/**
 * @brief The HistYColumnReader class
 */
class HistYColumnReader {
public:
    HistYColumnReader(
            const std::string& dir, const std::string& name,
            const std::string& iYColumnStr,
            const std::vector<std::string>& vars)
      : m_dir(dir), m_name(name), m_iYColumnStr(iYColumnStr), m_vars(vars) {}
    virtual ~HistYColumnReader() {}

public:
    std::vector<std::shared_ptr<HistDomain>> read() const;

private:
    std::string m_dir, m_name, m_iYColumnStr;
    std::vector<std::string> m_vars;
};

/**
 * @brief The HistFacadeYColumnReader class
 */
class HistFacadeYColumnReader {
public:
    HistFacadeYColumnReader(const std::string& dir, const std::string& name,
            const std::string& iYColumnStr,
            const std::vector<std::string>& vars)
      : m_dir(dir), m_name(name), m_iYColumnStr(iYColumnStr), m_vars(vars) {}

public:
    std::vector<std::shared_ptr<HistFacadeDomain>> read() const;

private:
    std::string m_dir, m_name, m_iYColumnStr;
    std::vector<std::string> m_vars;
};

#endif // HISTREADER_H
