#ifndef ZDICTCOMPRESS_H
#define ZDICTCOMPRESS_H

#include <QByteArray>
#include <QFile>

namespace ZDict {

QByteArray gzInflate(const QByteArray &src);

class DictFileData
{
public:
    bool isDictZip { false };
    int headerLength { 0 };
    quint16 chunkLength { 0U };
    qint16 chunkCount { 0 };
    QList<qint16> chunks;
    QList<quint64> offsets;

    DictFileData() {}
    void clear() {
        isDictZip = false;
        headerLength = 0;
        chunkLength = 0;
        chunkCount = 0;
        chunks.clear();
        offsets.clear();
    };
};

bool dictZipInitialize(QFile* dz, DictFileData* fileData);
QByteArray dictZipRead(QFile* dz, DictFileData* fileData, quint64 start, quint32 size);

}

#endif // ZDICTCOMPRESS_H
