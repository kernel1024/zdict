#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "zstardictdictionary.h"
#include "zdictcompress.h"

#include <QDebug>

namespace ZQDict {

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
    m_words.clear();

    QFile ifo(indexFile);
    if (!ifo.open(QIODevice::ReadOnly)) return false;

    QTextStream ifos(&ifo);

    int idxFileSize = 0;

    QString line;
    while (line.isEmpty() && !ifos.atEnd())
        line = ifos.readLine().trimmed();

    if (line != QSL("StarDict's dict ifo file")) {
        qWarning() << "Stardict: ifo signature not found.";
        return false;
    }

    while (!ifos.atEnd()) {
        line = ifos.readLine().trimmed();

        int eq = line.indexOf('=');
        if (eq<0) continue;

        const QString name = line.left(eq).trimmed().toLower();
        const QString param = line.mid(eq+1);

        if (name == QSL("wordcount")) {
            bool ok = false;
            int wc = param.toInt(&ok);
            if (ok)
                m_wordCount = wc;
        } else if (name == QSL("bookname")) {
            m_name = param;
        } else if (name == QSL("description")) {
            m_description = param;
        } else if (name == QSL("sametypesequence")) {
            if (!param.isEmpty())
                m_sameTypeSequence = param.at(0);
        } else if (name == QSL("idxoffsetbits")) {
            m_64bitOffset = (param == QSL("64"));
        } else if (name == QSL("idxfilesize")) {
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
        m_words.clear();
        return false;
    }

    return true;
}

bool ZStardictDictionary::loadStardictIndex(const QString &ifoFilename, unsigned int expectedIndexFileSize)
{
    QFileInfo fi(ifoFilename);
    const QString idxFilename = fi.dir().filePath(QSL("%1.%2").arg(fi.completeBaseName(),QSL("idx")));
    const QString gzIdxFilename = fi.dir().filePath(QSL("%1.%2").arg(fi.completeBaseName(),QSL("idx.gz")));

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

        m_index.insert(word.toLower(),qMakePair(offset,size));
        m_words.append(word.toLower());
        wordCounter++;
    }

    if (wordCounter!=m_wordCount)
        qWarning() << "Stardict: Unexpected dictionary word count.";

    return true;
}

bool ZStardictDictionary::loadStardictDict(const QString &ifoFilename)
{
    QFileInfo fi(ifoFilename);
    const QString dictFilename = fi.dir().filePath(QSL("%1.%2").arg(fi.completeBaseName(),QSL("dict")));
    const QString dzDictFilename = fi.dir().filePath(QSL("%1.%2").arg(fi.completeBaseName(),QSL("dict.dz")));

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
                                            const QRegularExpression& filter)
{
    QStringList res;

    if (m_words.contains(word))
        res.append(word);

    QRegularExpression rx = filter;
    if (rx.pattern().isEmpty()) {
        QString s = word;
        s.remove(QRegularExpression(QSL("\\W"),QRegularExpression::UseUnicodePropertiesOption));
        rx = QRegularExpression(QSL("^%1").arg(s));
    }
    rx.setPatternOptions(QRegularExpression::CaseInsensitiveOption |
                         QRegularExpression::UseUnicodePropertiesOption);

    if (rx.isValid() && !rx.pattern().isEmpty())
        res.append(m_words.filter(rx));

    return res;
}

QString ZStardictDictionary::loadArticle(const QString &word)
{
    QString res;

    const auto idxList = m_index.values(word);
    for (const auto& idx : idxList) {
        QByteArray article = dictZipRead(&m_dict,&m_dictData,idx.first,idx.second);

        res += QSL("\n<hr>\n%1:<br>\n%2").arg(getName(),QString::fromUtf8(article)); //TODO: format etc...

    }

    return res;

}

}
