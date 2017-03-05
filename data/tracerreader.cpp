#include "tracerreader.h"
#include <fstream>
#include <QFile>
#include <QElapsedTimer>
#include <QDebug>

std::ostream &operator<<(std::ostream &os, const Particle &p)
{
    os << "ssn:" << p.ssn
       << " loc:[" << p.loc.x << "," << p.loc.y << "," << p.loc.z
       << " xloc:[" << p.xloc.x << "," << p.xloc.y << "," << p.xloc.z
       << " vel:[" << p.vel.x << "," << p.vel.y << "," << p.vel.z
       << " rho:" << p.rho
       << " temp:" << p.temp
       << " mixfrac:" << p.mixfrac
       << " scaldis:" << p.scaldis;
    return os;
}

std::shared_ptr<TracerReader> TracerReader::create(const TracerConfig &config)
{
    std::ifstream fin;
    fin.open((config.dir() + "sortedtraceroffsets.00000").c_str(),
            std::ios::binary);
    if (fin) {
        return std::make_shared<SortedOrigTracerReader>(config);
    }
    fin.open(
            (config.dir() + "pdfsortedtracer.00000").c_str(), std::ios::binary);
    if (fin) {
        return std::make_shared<DomainSortedTracerReader>(config);
    }
    fin.open((config.dir() + "sortedid.00000").c_str(), std::ios::binary);
    if (fin) {
        return std::make_shared<ManyFilesTracerReader>(config);
    }
    fin.open((config.dir() + "tracer.00000").c_str(), std::ios::binary);
    if (fin) {
        return std::make_shared<OrigTracerReader>(config);
    }
    return nullptr;
}

std::vector<Particle> OrigTracerReader::read(
        const std::vector<int> &selectedHistFlatIds) const {
    std::vector<Particle> parts;
    // get the array of domains from histogram ids.
    std::unordered_map<int, std::vector<std::vector<int>>> dMap;
    for (auto histFlatId : selectedHistFlatIds) {
        auto ids = m_config.dimHists().flattoids(histFlatId);
        std::vector<int> dIds(3);
        std::vector<int> hIds(3);
        for (decltype(ids.size()) iDim = 0; iDim < ids.size(); ++iDim) {
            int nHistsPerDomain = m_config.dimHistsPerDomain()[iDim];
            dIds[iDim] = ids[iDim] / nHistsPerDomain;
            hIds[iDim] = ids[iDim] % nHistsPerDomain;
        }
        auto domainFlatId = m_config.dimDomains().idstoflat(dIds);
        dMap[domainFlatId].push_back(hIds);
    }
    // seperate the domain-histogram ids array into segments according to
    // yColumnIds.
    Extent yColumnsExtent(m_config.dimDomains()[0], m_config.dimDomains()[2]);
    std::unordered_map<int, DomainMap> yColumns;
    for (auto domain : dMap) {
        auto dIds = m_config.dimDomains().flattoids(domain.first);
        std::vector<int> yColumnIds = { dIds[0], dIds[2] };
        auto yColumnFlatId = yColumnsExtent.idstoflat(yColumnIds);
        yColumns[yColumnFlatId][domain.first] = domain.second;
    }
    // traverse the y columns.
    for (auto yColumn : yColumns) {
        int yColumnFlatId = yColumn.first;
        auto dMap = yColumn.second;
        auto yColumnParts = readYColumnDomains(yColumnFlatId, dMap);
        parts.insert(parts.end(), yColumnParts.begin(), yColumnParts.end());
    }
    return parts;
}

std::vector<Particle> OrigTracerReader::readYColumnDomains(
        int yColumnFlatId, const DomainMap &dMap) const
{
    std::vector<Particle> parts;
    // in each y column, binary search for the particles within each
    // sampling region.
    // using the flat y column id to construct the file name of the tracer.
    char filename[20];
    sprintf(filename, "tracer.%05d", yColumnFlatId);
    std::string filepath = m_config.dir() + filename;
    // search within the file for the particles of the selected domains.
    TracerFileReader reader(filepath);
    /// TODO: when length != endlen, swap endianness and try again.
    /* double time = */ reader.readDouble();
    int32_t fillsum = reader.readInt32();
    std::cout << "fillsum: " << fillsum << std::endl;
    reader.ignoreRecord(); // ssn_g
    reader.ignoreRecord(); // fill_g
    reader.ignoreRecord(); // seed_g
    /// TODO: currently assuming the order of the arrays, but the not needed
    /// arrays in the middle are ignored.
    std::vector<int64_t> ssn;
    std::vector<double> locx, locy, locz, xlocx, xlocy, xlocz, temp;
    while (!reader.readArrayIfNameIs<int64_t>("SSN", ssn)) {}
    while (!reader.readArrayIfNameIs<double>("loc1", locx)) {}
    while (!reader.readArrayIfNameIs<double>("xloc1", xlocx)) {}
    while (!reader.readArrayIfNameIs<double>("loc2", locy)) {}
    while (!reader.readArrayIfNameIs<double>("xloc2", xlocy)) {}
    while (!reader.readArrayIfNameIs<double>("loc3", locz)) {}
    while (!reader.readArrayIfNameIs<double>("xloc3", xlocz)) {}
    while (!reader.readArrayIfNameIs<double>("T", temp)) {}
    /// TODO: implement binary search instead of linear search.
    // loop through each particle and only save the ones that are in the
    // selected histograms.
    for (unsigned int i = 0; i < locx.size(); ++i) {
        // for each domain, check if loc is inside.
        for (auto d : dMap) {
            auto dIds = m_config.dimDomains().flattoids(d.first);
            float dx = m_config.dimVoxels()[0] / m_config.dimDomains()[0];
            float dy = m_config.dimVoxels()[1] / m_config.dimDomains()[1];
            float dz = m_config.dimVoxels()[2] / m_config.dimDomains()[2];
            auto dminx = dx * (dIds[0] + 0);
            auto dminy = dy * (dIds[1] + 0);
            auto dminz = dz * (dIds[2] + 0);
            auto dmaxx = dx * (dIds[0] + 1);
            auto dmaxy = dy * (dIds[1] + 1);
            auto dmaxz = dz * (dIds[2] + 1);
            Region dRegion(dminx, dminy, dminz, dmaxx, dmaxy, dmaxz);
            if (!dRegion.contains(locx[i], locy[i], locz[i])) continue;
            // for each sampling region, check if loc is inside.
            for (auto hIds : d.second) {
                float hx =
                        (dmaxx - dminx) / m_config.dimHistsPerDomain()[0];
                float hy =
                        (dmaxy - dminy) / m_config.dimHistsPerDomain()[1];
                float hz =
                        (dmaxz - dminz) / m_config.dimHistsPerDomain()[2];
                auto hminx = dminx + hx * (hIds[0] + 0);
                auto hminy = dminy + hy * (hIds[1] + 0);
                auto hminz = dminz + hz * (hIds[2] + 0);
                auto hmaxx = dminx + hx * (hIds[0] + 1);
                auto hmaxy = dminy + hy * (hIds[1] + 1);
                auto hmaxz = dminz + hz * (hIds[2] + 1);
                Region hRegion(hminx, hminy, hminz, hmaxx, hmaxy, hmaxz);
                if (!hRegion.contains(locx[i], locy[i], locz[i])) continue;
                // add the particles to the return array.
                Particle part;
                part.ssn = ssn[i];
                part.loc[0] = locx[i];
                part.loc[1] = locy[i];
                part.loc[2] = locz[i];
                part.xloc[0] = xlocx[i];
                part.xloc[1] = xlocy[i];
                part.xloc[2] = xlocz[i];
                part.temp = temp[i];
                parts.push_back(part);
            }
        }
    }
    return parts;
}

std::vector<Particle> SortedOrigTracerReader::readYColumnDomains(
        int yColumnFlatId, const DomainMap &dMap) const {
    std::vector<Particle> parts;
    // index file name
    char indexFileName[100];
    sprintf(indexFileName, "sortedtraceroffsets.%05d", yColumnFlatId);
    std::string indexFilePath = m_config.dir() + indexFileName;
    // read the index file
    FortranReader indexReader(indexFilePath);
    std::vector<int32_t> offsets = indexReader.readInt32Array();
    parts.reserve(offsets[offsets.size() - 2] + offsets[offsets.size() - 1]);
    // tracer file name
    char tracerFileName[20];
    sprintf(tracerFileName, "tracer.%05d", yColumnFlatId);
    std::string tracerFilePath = m_config.dir() + tracerFileName;
    // read files from the tracer file by indexing with the index file
    TracerFileReader tracerReader(tracerFilePath);
    /* double time = */ tracerReader.readDouble();
    /* int32_t fillsum = */ tracerReader.readInt32();
    tracerReader.ignoreRecord(); // ssn_g
    tracerReader.ignoreRecord(); // fill_g
    tracerReader.ignoreRecord(); // seed_g
    int dataArraysReadPos = tracerReader.currReadPos();
    /// TODO: currently assuming the order of the arrays, but the not needed
    /// arrays in the middle are ignored.
    for (auto d : dMap) {
        auto dIds = m_config.dimDomains().flattoids(d.first);
        for (auto localHistIds : d.second) {
            int nHistsPerDomain = m_config.dimHistsPerDomain()[0] *
                    m_config.dimHistsPerDomain()[1] *
                    m_config.dimHistsPerDomain()[2];
            int histOffset = dIds[1] * nHistsPerDomain;
            int flatLocalHistId =
                    m_config.dimHistsPerDomain().idstoflat(localHistIds);
            int flatYColumnHistId = histOffset + flatLocalHistId;
            int offset = offsets[2 * flatYColumnHistId + 0];
            int nParts = offsets[2 * flatYColumnHistId + 1];
            std::vector<int64_t> ssn;
            std::vector<double> locx, locy, locz, xlocx, xlocy, xlocz;
            std::vector<float> temp;
            tracerReader.setReadPosFromBeg(dataArraysReadPos);
            while (!tracerReader.readSubArrayIfNameIs<int64_t>(
                    "SSN", offset, nParts, ssn)) {}
            while (!tracerReader.readSubArrayIfNameIs<double>(
                    "loc1", offset, nParts, locx)) {}
            while (!tracerReader.readSubArrayIfNameIs<double>(
                    "xloc1", offset, nParts, xlocx)) {}
            while (!tracerReader.readSubArrayIfNameIs<double>(
                    "loc2", offset, nParts, locy)) {}
            while (!tracerReader.readSubArrayIfNameIs<double>(
                    "xloc2", offset, nParts, xlocy)) {}
            while (!tracerReader.readSubArrayIfNameIs<double>(
                    "loc3", offset, nParts, locz)) {}
            while (!tracerReader.readSubArrayIfNameIs<double>(
                    "xloc3", offset, nParts, xlocz)) {}
            while (!tracerReader.readSubArrayIfNameIs<float>(
                    "T", offset, nParts, temp)) {}
            for (auto iPart = 0; iPart < nParts; ++iPart) {
                Particle part;
                part.ssn = ssn[iPart];
                part.loc[0] = locx[iPart];
                part.loc[1] = locy[iPart];
                part.loc[2] = locz[iPart];
                part.xloc[0] = xlocx[iPart];
                part.xloc[1] = xlocy[iPart];
                part.xloc[2] = xlocz[iPart];
                part.temp = temp[iPart];
                parts.push_back(part);
            }
        }
    }
    return parts;
}

std::vector<Particle> DomainTracerReader::read(
        const std::vector<int> &selectedHistFlatIds) const
{
    std::vector<Particle> parts;
    QElapsedTimer timer;
    timer.start();
    for (unsigned int iSelect = 0; iSelect < selectedHistFlatIds.size(); ++iSelect) {
        auto ids = m_config.dimHists().flattoids(selectedHistFlatIds[iSelect]);
        std::vector<int> dIds(ids.size());
        std::vector<int> hIds(ids.size());
        for (unsigned int iDim = 0; iDim < ids.size(); ++iDim) {
            int nHistsPerDomain = m_config.dimHistsPerDomain()[iDim];
            dIds[iDim] = ids[iDim] / nHistsPerDomain;
            hIds[iDim] = ids[iDim] % nHistsPerDomain;
        }
        int dId = m_config.dimDomains().idstoflat(dIds);
        int hId = m_config.dimHistsPerDomain().idstoflat(hIds);
        auto local = readLocal(m_config.dir(), dId, hId);
        parts.insert(parts.end(), local.begin(), local.end());
        qDebug() << "loaded" << parts.size() << "in" << timer.elapsed() <<"ms.";
    }
    return parts;
}

std::vector<Particle> ManyFilesTracerReader::readLocal(const std::string &dir, int dId, int hId) const
{
    std::vector<Particle> parts;
    // file names
    QString data_path = QString::fromStdString(dir);
    QString helper_filename = data_path + QString("/sortedhelper.%1").arg(dId, 5, 10, QChar('0'));
    QString id_filename = data_path + QString("/sortedid.%1").arg(dId, 5, 10, QChar('0'));
    QString pos_filename = data_path + QString("/sortedposition.%1").arg(dId, 5, 10, QChar('0'));
    QString data_filename = data_path + QString("/sortedtracer.%1").arg(dId, 5, 10, QChar('0'));

    int read;
    int count;

    // open helper, go to hId, get start read and count
    QFile helper_file(helper_filename);
    if (!helper_file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't find or open" << helper_file.fileName();
        return parts;
    }

    helper_file.seek(hId * 2 * sizeof(int));
    helper_file.read((char*)&read, sizeof(int));
    helper_file.read((char*)&count, sizeof(int));
    helper_file.close();

    // open rest of the particle files
    QFile id_file(id_filename);
    if (!id_file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't find or open" << id_file.fileName();
        return parts;
    }
    QFile pos_file(pos_filename);
    if (!pos_file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't find or open" << pos_file.fileName();
        return parts;
    }
    QFile data_file(data_filename);
    if (!data_file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't find or open" << data_file.fileName();
        return parts;
    }

    // grab the corresponding particles
    if (0 != count) {
        id_file.seek(read * sizeof(long long int));
        pos_file.seek(read * 3 * sizeof(double));
        data_file.seek(read * sizeof(double));
        for (int i = 0; i < count; ++i) {
            Particle part;
            long int id;
            double x, y, z, v;
            id_file.read((char*)&id, sizeof(long long int));
            pos_file.read((char*)&x, sizeof(double));
            pos_file.read((char*)&y, sizeof(double));
            pos_file.read((char*)&z, sizeof(double));
            data_file.read((char*)&v, sizeof(double));
            part.ssn = id;
            part.xloc[0] = x;
            part.xloc[1] = y;
            part.xloc[2] = z;
            part.temp = v;
            parts.push_back(part);
        }
    }

    // close the files
    id_file.close();
    pos_file.close();
    data_file.close();

    return parts;
}

std::vector<Particle> DomainSortedTracerReader::readLocal(
        const std::string &dir, int dId, int hId) const
{
    std::vector<Particle> parts;
    std::string filename =
            QString("pdfsortedtracer.%1")
                .arg(dId, 5, 10, QChar('0')).toStdString();
    std::string filepath = dir + filename;
    std::ifstream fin(filepath.c_str(), std::ios::binary);
    // read the number of histograms per domain decomposition
    std::vector<int32_t> dimHists(3);
    fin.read(reinterpret_cast<char*>(dimHists.data()), 3 * sizeof(int));
    int nHists = dimHists[0] * dimHists[1] * dimHists[2];
    int startOfPart = 3 * sizeof(int32_t) + 2 * nHists * sizeof(int32_t);
    // seek to the corresponding histogram offset and count
    fin.seekg(3 * sizeof(int32_t) + hId * 2 * sizeof(int32_t), fin.beg);
    // read the offset and count
    int32_t offset, count;
    fin.read(reinterpret_cast<char*>(&offset), sizeof(int32_t));
    fin.read(reinterpret_cast<char*>(&count), sizeof(int32_t));
    int32_t lastOffset, lastCount;
    fin.seekg(startOfPart - 2 * sizeof(int32_t), fin.beg);
    fin.read(reinterpret_cast<char*>(&lastOffset), sizeof(int32_t));
    fin.read(reinterpret_cast<char*>(&lastCount), sizeof(int32_t));
    int32_t totalCount = lastOffset + lastCount;
    // return if there is no histogram in the sampling region
    if (0 == count) return std::vector<Particle>();
    // read the corresponding particles
    // read the particle id
    fin.seekg(startOfPart + offset * sizeof(int64_t), fin.beg);
    std::vector<int64_t> partIds(count);
    fin.read(reinterpret_cast<char*>(partIds.data()), count * sizeof(int64_t));
    // read the particle physical positions
    fin.seekg(startOfPart + totalCount * sizeof(int64_t) + offset * 3 * sizeof(double), fin.beg);
    std::vector<double> xlocs(3 * count);
    fin.read(reinterpret_cast<char*>(xlocs.data()), count * 3 * sizeof(double));
    // read the particle scalar values
    fin.seekg(startOfPart + totalCount * (sizeof(int64_t) + 3 * sizeof(double)) + offset * sizeof(double));
    std::vector<double> scalars(count);
    fin.read(reinterpret_cast<char*>(scalars.data()), count * sizeof(double));
    // put the loaded arrays into the return array
    parts.resize(count);
    for (int iPart = 0; iPart < count; ++iPart) {
        auto& part = parts[iPart];
        part.ssn = partIds[iPart];
        part.xloc[0] = xlocs[3 * iPart + 0];
        part.xloc[1] = xlocs[3 * iPart + 1];
        part.xloc[2] = xlocs[3 * iPart + 2];
        part.temp = scalars[iPart];
    }
    return parts;
}
