#ifndef ZSTARDICTDICTIONARY_H
#define ZSTARDICTDICTIONARY_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include "zdictionary.h"
#include "zdictcompress.h"

namespace ZQDict {

class ZStardictDictionary : public ZDictionary
{
    Q_OBJECT

    friend class ZDictController;
private:
    QMap<QString,QPair<quint64,quint32> > m_index;
    QStringList m_words;

    QFile m_dict;
    DictFileData m_dictData;

    QString m_name;
    QString m_description;
    int m_wordCount { -1 };
    QChar m_sameTypeSequence;
    bool m_64bitOffset { false };

    bool loadStardictIndex(const QString& ifoFilename, unsigned int expectedIndexFileSize);
    bool loadStardictDict(const QString& ifoFilename);

public:
    ZStardictDictionary(QObject *parent = nullptr);
    ~ZStardictDictionary() override;

protected:
    bool loadIndexes(const QString& indexFile) override;
    QStringList wordLookup(const QString& word,
                           const QRegularExpression& filter = QRegularExpression()) override;
    QString loadArticle(const QString& word) override;
    QString getName() override { return m_name; };
    QString getDescription() override { return m_description; };
    int getWordCount() override { return m_wordCount; };

};

}

#endif // ZSTARDICTDICTIONARY_H
