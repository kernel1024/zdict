#ifndef ZDICTIONARY_H
#define ZDICTIONARY_H

#include <QObject>
#include <QStringList>
#include <QRegularExpression>

namespace ZDict {

#define QSL QStringLiteral

class ZDictController;

class ZDictionary : public QObject
{
    friend class ZDictController;

    Q_OBJECT
public:
    ZDictionary(QObject *parent = nullptr);
    ~ZDictionary() override;

protected:
    virtual bool loadIndexes(const QString& indexFile) = 0;
    virtual QStringList wordLookup(const QString& word,
                                   const QRegularExpression& filter = QRegularExpression()) = 0;
    virtual QString loadArticle(const QString& word) = 0;
    virtual QString getName() = 0;
    virtual QString getDescription() = 0;
    virtual int getWordCount() = 0;

};

}

#endif // ZDICTIONARY_H
