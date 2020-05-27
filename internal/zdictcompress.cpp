#include <QDebug>

extern "C" {
#include <zlib.h>
}

#include "zdictcompress.h"
#include "zdictionary.h"

namespace ZDict {

QByteArray gzInflate(const QByteArray &src)
{
    QByteArray res;

    const int chunkSize = 1024;
    char buf[chunkSize] = {};

    z_stream strm;
    int ret;

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = src.size();
    strm.next_in = reinterpret_cast<uchar*>(const_cast<char*>(src.constData()));

    ret = inflateInit2(&strm, 15+32);
    if (ret != Z_OK)
        return res;

    do {
        strm.avail_out = chunkSize;
        strm.next_out = reinterpret_cast<uchar*>(buf);
        ret = inflate(&strm,Z_NO_FLUSH);

        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            return QByteArray();
        }

        res.append(buf, chunkSize - strm.avail_out);
    } while (strm.avail_out == 0);

    inflateEnd(&strm);

    return res;
}

bool dictZipInitialize(QFile* dz, DictFileData* fileData)
{
    fileData->clear();

    const QByteArray GZ_MAGIC = QByteArrayLiteral("\x1f\x8b");
    const QByteArray DZ_MAGIC = QByteArrayLiteral("RA");

    const char GZ_FEXTRA = 0x04;
    const char GZ_FNAME = 0x08;
    const char GZ_COMMENT = 0x10;
    const char GZ_FHCRC = 0x02;

    const int GZ_XLEN = 10;
    const int GZ_MAXBUF = 10240;

    dz->seek(0L);
    QByteArray buf;

    buf = dz->read(2);
    if (buf != GZ_MAGIC) {
        qWarning() << "DictZIP: GZ magic not found.";
        return false;
    }

    dz->skip(1); // skip method

    char gzFlags = 0;
    dz->read(&gzFlags,sizeof(gzFlags));

    if ((gzFlags & GZ_FEXTRA) == 0) {
        qWarning() << "DictZIP: not a dictZip file (no extra flags).";
        return false;
    }

    dz->skip(6); // skip remained header
    qint16 extraLength = 0;
    dz->read(reinterpret_cast<char*>(&extraLength),sizeof(extraLength));

    fileData->headerLength = GZ_XLEN - 1 + extraLength + 2;

    buf = dz->read(2);
    if (buf != DZ_MAGIC) {
        qWarning() << "DictZIP: not a dictZip file (no DZ magic).";
        return false;
    }

    dz->skip(4); // sublength, version

    dz->read(reinterpret_cast<char*>(&(fileData->chunkLength)),sizeof(fileData->chunkLength));
    dz->read(reinterpret_cast<char*>(&(fileData->chunkCount)),sizeof(fileData->chunkCount));

    if (fileData->chunkCount <= 0) {
        qWarning() << "DictZIP: broken dictZip file (no chunks).";
        return false;
    }

    for (int i=0; i<fileData->chunkCount; i++) {
        qint16 chunk = 0;
        dz->read(reinterpret_cast<char*>(&chunk),sizeof(chunk));
        fileData->chunks.append(chunk);
    }

    if (gzFlags & GZ_FNAME) {
        char cbuf = 0xff;
        int start = dz->pos();
        while (cbuf != 0x0 && !dz->atEnd() && ((dz->pos()-start)<GZ_MAXBUF)) {
            dz->read(&cbuf,sizeof(cbuf));
        }
        if ((cbuf != 0x0) || dz->atEnd()) {
            qWarning() << "DictZIP: broken dictZip file (end of file in headers - filename).";
            return false;
        }

        fileData->headerLength += dz->pos() - start;
    }

    if (gzFlags & GZ_COMMENT) {
        char cbuf = 0xff;
        int start = dz->pos();
        while (cbuf != 0x0 && !dz->atEnd() && ((dz->pos()-start)<GZ_MAXBUF)) {
            dz->read(&cbuf,sizeof(cbuf));
        }
        if ((cbuf != 0x0) || dz->atEnd()) {
            qWarning() << "DictZIP: broken dictZip file (end of file in headers - comment).";
            return false;
        }

        fileData->headerLength += dz->pos() - start;
    }

    if (gzFlags & GZ_FHCRC) {
        dz->skip(2); // skip CRC16
        fileData->headerLength += 2;
    }

    if (dz->pos() != (fileData->headerLength+1)) {
        qWarning() << "DictZIP: broken dictZip file (file position != headerLength+1).";
        return false;
    }

    quint64 offset = fileData->headerLength + 1;
    for (int i=0; i<fileData->chunkCount; i++)
    {
        fileData->offsets.append(offset);
        offset += fileData->chunks.at(i);
    }

    fileData->isDictZip = true;

    return true;
}

QByteArray dictZipRead(QFile* dz, DictFileData* fileData, quint64 start, quint32 size)
{
    const int IN_BUFFER_SIZE = 60000;

    QByteArray res;

    if (!dz->isOpen())
        return res;

    if (!fileData->isDictZip) {
        dz->seek(start);
        return dz->read(size);
    }

    z_stream strm;
    int ret;

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;
    strm.avail_out = 0;
    strm.next_out = nullptr;

    ret = inflateInit2(&strm, -15);
    if (ret != Z_OK)
        return res;

    quint64 end = start + size;

    int firstChunk  = start / fileData->chunkLength;
    int firstOffset = start - firstChunk * fileData->chunkLength;
    int lastChunk   = end / fileData->chunkLength;
    int lastOffset  = end - lastChunk * fileData->chunkLength;

    QByteArray inBuffer(IN_BUFFER_SIZE, 0x0);

    for (int i=firstChunk; i <= lastChunk; i++) {
        dz->seek(fileData->offsets.at(i));
        QByteArray outBuffer = dz->read(fileData->chunks.at(i));
        if (outBuffer.size() != fileData->chunks.at(i)) {
            qWarning() << "DictZIP: chunk read error";
            res.clear();
            break;
        }

        strm.next_in   = reinterpret_cast<Bytef *>(outBuffer.data());
        strm.avail_in  = fileData->chunks.at(i);
        strm.next_out  = reinterpret_cast<Bytef *>(inBuffer.data());
        strm.avail_out = IN_BUFFER_SIZE;
        if (inflate(&strm,  Z_PARTIAL_FLUSH ) != Z_OK) {
            qWarning() << QSL("DictZIP: zlib inflate error: %1").arg(QString::fromUtf8(strm.msg));
            res.clear();
            break;
        }
        if (strm.avail_in) {
            qWarning() << QSL("DictZIP: inflate did not flush (%1 pending, %2 avail)")
                          .arg(strm.avail_in).arg(strm.avail_out);
            res.clear();
            break;
        }

        int count = IN_BUFFER_SIZE - strm.avail_out;

        if (i == firstChunk) {
            if (i == lastChunk) {
                res.append(inBuffer.mid(firstOffset, lastOffset - firstOffset));
            } else {
                if (count != fileData->chunkLength) {
                    qWarning() << QSL("DictZIP: Length = %1 instead of %2")
                                  .arg(count).arg(fileData->chunkLength);
                }
                res.append(inBuffer.mid(firstOffset, fileData->chunkLength - firstOffset));
            }
        } else if (i == lastChunk) {
            res.append(inBuffer.mid(0,lastOffset));
        } else {
            res.append(inBuffer.mid(0,fileData->chunkLength));
        }
    }

    inflateEnd(&strm);

    return res;
}

}
