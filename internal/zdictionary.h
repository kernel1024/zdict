#ifndef ZDICTIONARY_H
#define ZDICTIONARY_H

#include <QStringList>
#include <QRegularExpression>
#include <QAtomicInteger>

namespace ZDict {

const int defaultMaxLookupWords = 10000;

#define ZDQSL QStringLiteral // NOLINT

class ZDictController;

class ZDictionary
{
    friend class ZDictController;

private:
    QAtomicInteger<bool> m_stopRequest;

public:
    ZDictionary() = default;
    virtual ~ZDictionary() = default;
    ZDictionary(const ZDictionary& other) = delete;
    ZDictionary& operator = (const ZDictionary &t) = delete;

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
