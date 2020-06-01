#ifndef ZDICTIONARY_H
#define ZDICTIONARY_H

#include <QObject>
#include <QStringList>
#include <QRegularExpression>
#include <QAtomicInteger>

namespace ZDict {

const int defaultMaxLookupWords = 10000;

#define ZDQSL QStringLiteral // NOLINT

class ZDictController;

class ZDictionary : public QObject
{
    friend class ZDictController;

    Q_OBJECT
private:
    QAtomicInteger<bool> m_stopRequest;

public:
    ZDictionary(QObject *parent = nullptr);
    ~ZDictionary() override;

    inline void resetStopRequest() { m_stopRequest.storeRelease(false); }
    inline void stopRequest() { m_stopRequest.storeRelease(true); }

protected:
    virtual bool loadIndexes(const QString& indexFile) = 0;
    virtual QStringList wordLookup(const QString& word,
                                   bool suppressMultiforms = false,
                                   int maxLookupWords = defaultMaxLookupWords) = 0;
    virtual QString loadArticle(const QString& word) = 0;
    virtual QString getName() = 0;
    virtual QString getDescription() = 0;
    virtual int getWordCount() = 0;

    inline bool isStopRequested() { return m_stopRequest.loadAcquire(); }

};

}

#endif // ZDICTIONARY_H
