#ifndef ZSTARDICTDICTIONARY_H
#define ZSTARDICTDICTIONARY_H

#include <QObject>
#include <QStringList>
#include <QMultiMap>
#include <QVector>
#include "zdictionary.h"
#include "zdictcompress.h"

namespace ZDict {

using ZStardictIndex = QMultiMap<QString,QPair<quint64,quint32> >;

class ZStardictDictionary : public ZDictionary
{
    Q_OBJECT

    friend class ZDictController;
private:
    ZStardictIndex m_index;

    QFile m_dict;
    DictFileData m_dictData;

    QString m_name;
    QString m_description;
    int m_wordCount { -1 };
    QString m_sameTypeSequence;
    bool m_64bitOffset { false };

    bool loadStardictIndex(const QString& ifoFilename, unsigned int expectedIndexFileSize);
    bool loadStardictDict(const QString& ifoFilename);
    QString handleResource(QChar type, const char *data, quint32 size);

public:
    ZStardictDictionary(QObject *parent = nullptr);
    ~ZStardictDictionary() override;

protected:
    bool loadIndexes(const QString& indexFile) override;
    QStringList wordLookup(const QString& word,
                           bool suppressMultiforms = false,
                           int maxLookupWords = defaultMaxLookupWords) override;
    QString loadArticle(const QString& word) override;
    QString getName() override { return m_name; };
    QString getDescription() override { return m_description; };
    int getWordCount() override { return m_wordCount; };

};

}

#endif // ZSTARDICTDICTIONARY_H
