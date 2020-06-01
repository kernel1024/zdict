/*
 * Parts of this file taken from:
 * - GoldenDict. Licensed under GPLv3 or later, see the LICENSE file.
 *   (c) 2008-2011 Konstantin Isakov <ikm@users.berlios.de>
 */

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "zstardictdictionary.h"
#include "zdictcompress.h"
#include "zdictconversions.h"

#include <QDebug>

namespace ZDict {

ZStardictDictionary::ZStardictDictionary(QObject *parent)
    : ZDictionary(parent)
{
}

ZStardictDictionary::~ZStardictDictionary()
{
    if (m_dict.isOpen())
        m_dict.close();
}

bool ZStardictDictionary::loadIndexes(const QString &indexFile)
{
    m_index.clear();

    QFile ifo(indexFile);
    if (!ifo.open(QIODevice::ReadOnly)) return false;

    QTextStream ifos(&ifo);

    int idxFileSize = 0;

    QString line;
    while (line.isEmpty() && !ifos.atEnd())
        line = ifos.readLine().trimmed();

    if (line != ZDQSL("StarDict's dict ifo file")) {
        qWarning() << "Stardict: ifo signature not found.";
        return false;
    }

    while (!ifos.atEnd()) {
        line = ifos.readLine().trimmed();

        int eq = line.indexOf('=');
        if (eq<0) continue;

        const QString name = line.left(eq).trimmed().toLower();
        const QString param = line.mid(eq+1);

        if (name == ZDQSL("wordcount")) {
            bool ok = false;
            int wc = param.toInt(&ok);
            if (ok)
                m_wordCount = wc;
        } else if (name == ZDQSL("bookname")) {
            m_name = param;
        } else if (name == ZDQSL("description")) {
            m_description = param;
        } else if (name == ZDQSL("sametypesequence")) {
            m_sameTypeSequence = param;
        } else if (name == ZDQSL("idxoffsetbits")) {
            m_64bitOffset = (param == ZDQSL("64"));
        } else if (name == ZDQSL("idxfilesize")) {
            idxFileSize = param.toULongLong();
        }
    }

    if (m_name.isEmpty() || (m_wordCount<0) || (idxFileSize == 0)) {
        qWarning() << "Stardict: Incomplete IFO file.";
        return false;
    }

    if (!loadStardictIndex(indexFile,idxFileSize)) {
        qWarning() << "Stardict: Unable to load IDX file.";
        return false;
    }

    if (!loadStardictDict(indexFile)) {
        qWarning() << "Stardict: Unable to load DICT file.";
        m_index.clear();
        return false;
    }

    return true;
}

bool ZStardictDictionary::loadStardictIndex(const QString &ifoFilename, unsigned int expectedIndexFileSize)
{
    const QRegularExpression rxSplitter(ZDQSL("[\\s[:punct:]]"),
                                        QRegularExpression::UseUnicodePropertiesOption);

    QFileInfo fi(ifoFilename);
    const QString idxFilename = fi.dir().filePath(ZDQSL("%1.%2").arg(fi.completeBaseName(),ZDQSL("idx")));
    const QString gzIdxFilename = fi.dir().filePath(ZDQSL("%1.%2").arg(fi.completeBaseName(),ZDQSL("idx.gz")));

    bool gz = false;
    QFile idx(idxFilename);
    if (!idx.exists()) {
        idx.setFileName(gzIdxFilename);
        if (!idx.exists()) {
            qWarning() << "Stardict: IDX file not found.";
            return false;
        }
        gz = true;
    }
    if (!idx.open(QIODevice::ReadOnly)) {
        qWarning() << "Stardict: IDX file unable to open.";
        return false;
    }
    QByteArray binidx = idx.readAll();
    idx.close();
    if (gz)
        binidx = gzInflate(binidx);

    if (static_cast<unsigned int>(binidx.size()) != expectedIndexFileSize) {
        qWarning() << "Stardict: unexpected IDX file size.";
        return false;
    }

    binidx.append(QChar(0x0));
    int wordCounter = 0;
    for (auto it = binidx.constBegin(), end = binidx.constEnd(); it<(end-1);) {
        int wordLen = strlen(it);
        if ((it+wordLen+1+sizeof(quint32)*2)>end) {
            //qWarning() << "Stardict: unexpected end of IDX file.";
            break;
        }
        QString word = QString::fromUtf8(it,wordLen);
        it += wordLen + 1;
        quint64 offset = 0U;
        if (m_64bitOffset) {
            offset = be64toh(*(reinterpret_cast<quint64*>(const_cast<char*>(it))));
            it += sizeof(quint64);
        } else {
            quint32 offset32 = be32toh(*(reinterpret_cast<quint32*>(const_cast<char*>(it))));
            it += sizeof(quint32);
            offset = offset32;
        }
        quint32 size = be32toh(*(reinterpret_cast<quint32*>(const_cast<char*>(it))));
        it += sizeof(quint32);

        // split complex form by whitespaces/punctuation
        const QStringList sl = word.split(rxSplitter,Qt::SkipEmptyParts);
        for (const auto& s : sl)
            m_index.insert(s.toLower(),qMakePair(offset,size));

        if (sl.count()>1) // add complex form itself
            m_index.insert(word,qMakePair(offset,size));

        wordCounter++;
    }

    if (wordCounter!=m_wordCount)
        qWarning() << "Stardict: Unexpected dictionary word count.";

    return true;
}

bool ZStardictDictionary::loadStardictDict(const QString &ifoFilename)
{
    QFileInfo fi(ifoFilename);
    const QString dictFilename = fi.dir().filePath(ZDQSL("%1.%2").arg(fi.completeBaseName(),ZDQSL("dict")));
    const QString dzDictFilename = fi.dir().filePath(ZDQSL("%1.%2").arg(fi.completeBaseName(),ZDQSL("dict.dz")));

    m_dictData.clear();
    m_dict.setFileName(dictFilename);
    if (m_dict.open(QIODevice::ReadOnly)) return true;

    m_dict.setFileName(dzDictFilename);
    if (!m_dict.open(QIODevice::ReadOnly)) {
        qWarning() << "Stardict: unable to open DICT.DZ file.";
        return false;
    }
    if (!dictZipInitialize(&m_dict,&m_dictData) || (!m_dictData.isDictZip)) {
        qWarning() << "Stardict: unable to initialize DictZIP structures on DICT.DZ file.";
        return false;
    }

    return true;
}

QStringList ZStardictDictionary::wordLookup(const QString& word,
                                            bool suppressMultiforms,
                                            int maxLookupWords)
{
    QStringList res;
    if (isStopRequested())
        return res;

    QList<quint64> usedArticles;
    auto it = qAsConst(m_index).lowerBound(word);

    if (it == m_index.constEnd()) return res;
    if (!it.key().startsWith(word)) // nothing similar found in sorted key list
        return res;

    while ((it != m_index.constEnd()) && (res.count()<maxLookupWords) && (!isStopRequested())) {
        if (it.key().startsWith(word)) {
            if (!suppressMultiforms || !usedArticles.contains(it.value().first)) {
                res.append(it.key());
                if (suppressMultiforms)
                    usedArticles.append(it.value().first);
            }
        }
        it++;
    }

    return res;
}

QString ZStardictDictionary::handleResource(QChar type, const char *data, quint32 size)
{
    if (type == QChar('x')) // Xdxf content
        return ZDictConversions::xdxf2Html(QString::fromUtf8(data,size));

    if ((type == QChar('h') || (type == QChar('g')))) // Html content or Pango markup
        return QString::fromUtf8(data, size );

    if (type == QChar('m')) // Pure meaning, usually means preformatted text
        return ZDictConversions::htmlPreformat(QString::fromUtf8(data,size));

    if (type == QChar('l')) // Same as 'm', but not in utf8, instead in current locale's
        return ZDictConversions::htmlPreformat(QString::fromLocal8Bit(data,size));

    if (type.isLower()) {
        return ZDQSL("<b>Unsupported textual entry type '%1': %2.</b><br>" )
                .arg(type).arg(QString::fromUtf8(data,size).toHtmlEscaped());
    }

    return ZDQSL("<b>Unsupported blob entry type '%1'.</b><br>" ).arg(type);
}

QString ZStardictDictionary::loadArticle(const QString &word)
{
    QString res;

    const auto idxList = m_index.values(word); // NOLINT
    for (const auto& idx : idxList) {
        if (isStopRequested())
            return res;

        if (!res.isEmpty())
            res.append(ZDQSL("<br/><b>%1</b>").arg(word));

        quint64 offset = idx.first;
        quint32 size = idx.second;

        QByteArray article = dictZipRead(&m_dict,&m_dictData,offset,size);

        QString articleText;
        const auto *it = article.constBegin();

        if (!m_sameTypeSequence.isEmpty()) {
            for (int seq=0; seq<m_sameTypeSequence.length(); seq++) {
                bool entrySizeKnown = (seq == (m_sameTypeSequence.length() - 1));

                uint32_t entrySize = 0;

                if (entrySizeKnown) {
                    entrySize = size;
                } else if (size == 0) {
                    qWarning() << "Short entry for the word encountered";
                    break;
                }

                QChar type = m_sameTypeSequence.at(seq);

                if (type.isLower()) {
                    // Zero-terminated entry, unless it's the last one
                    if (!entrySizeKnown)
                        entrySize = strlen(it);

                    if (size < entrySize) {
                        qWarning() << "Malformed entry for the word encountered. ";
                        break;
                    }

                    articleText += handleResource( type, it, entrySize );

                    if ( !entrySizeKnown )
                        ++entrySize; // Need to skip the zero byte
                    it += entrySize;
                    size -= entrySize;

                } else if (type.isUpper()) {
                    // An entry which has its size before contents, unless it's the last one
                    if (!entrySizeKnown) {
                        if (size < sizeof(quint32)) {
                            qWarning() << "Malformed entry for the word encountered";
                            break;
                        }

                        entrySize = be32toh(*(reinterpret_cast<quint32*>(const_cast<char*>(it))));
                        it += sizeof(quint32);
                        size -= sizeof( quint32 );
                    }

                    if ( size < entrySize ) {
                        qWarning() << "Malformed entry for the word encountered: ";
                        break;
                    }

                    articleText += handleResource( type, it, entrySize );
                    it += entrySize;
                    size -= entrySize;
                } else {
                    qWarning() << "Non-alpha entry type " << type;
                    break;
                }
            }
        } else {
            // The sequence is stored in each article separately
            while (size>0)
            {
                QChar type(*it);
                if (type.isLower()) {
                    // Zero-terminated entry
                    size_t len = strlen(it + 1);

                    if (size < len + 2) {
                        qWarning() << "Malformed entry for the word encountered";
                        break;
                    }

                    articleText += handleResource(*it, it + 1, len);
                    it += len + 2;
                    size -= len + 2;
                } else if (type.isUpper()) {
                    // An entry which havs its size before contents
                    if ( size < sizeof(quint32) + 1 ) {
                        qWarning() << "Malformed entry for the word encountered:";
                        break;
                    }

                    quint32 entrySize = be32toh(*(reinterpret_cast<quint32*>(const_cast<char*>(it+1))));
                    if (size < sizeof( uint32_t ) + 1 + entrySize) {
                        qWarning() << "Malformed entry for the word encountered";
                        break;
                    }

                    articleText += handleResource( *it, it + 1 + sizeof( uint32_t ), entrySize );
                    it += sizeof( uint32_t ) + 1 + entrySize;
                    size -= sizeof( uint32_t ) + 1 + entrySize;
                } else {
                    qWarning() << "Non-alpha entry type encountered " << type;
                    break;
                }
            }
        }

        res += articleText;
    }

    return res;

}

}
