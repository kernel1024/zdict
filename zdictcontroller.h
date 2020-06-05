#ifndef ZDICTCONTROLLER_H
#define ZDICTCONTROLLER_H

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QMutex>

#include "internal/zdictionary.h"

namespace ZDict {

class ZDictController : public QObject
{
    Q_OBJECT
private:
    QVector<QSharedPointer<ZDictionary> > m_dicts;
    QMutex m_dictsMutex;
    QAtomicInteger<bool> m_loaded;

public:
    explicit ZDictController(QObject *parent = nullptr);
    ~ZDictController() override;

    void setMaxLookupWords(int maxLookupWords);
    QStringList getLoadedDictionaries() const;
    void loadDictionaries(const QStringList& pathList);

    QStringList wordLookup(const QString& word,
                           bool suppressMultiforms = false,
                           int maxLookupWords = defaultMaxLookupWords);
    void wordLookupAsync(const QString& word,
                         bool suppressMultiforms = false,
                         int maxLookupWords = defaultMaxLookupWords);

    QString loadArticle(const QString& word, bool addDictionaryName = true);
    void loadArticleAsync(const QString& word, bool addDictionaryName = true);

Q_SIGNALS:
    void wordListComplete(const QStringList& words); // cross-thread signal, use queued connect!
    void articleComplete(const QString& article); // cross-thread signal, use queued connect!
    void dictionariesLoaded(const QString& message); // cross-thread signal, use queued connect!

public Q_SLOTS:
    void cancelActiveWork();

};

}

#endif // ZDICTCONTROLLER_H
