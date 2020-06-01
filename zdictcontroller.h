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
    QVector<QPointer<ZDictionary> > m_dicts;
    QMutex m_dictsMutex;
    QAtomicInteger<bool> m_loaded;

public:
    explicit ZDictController(QObject *parent = nullptr);
    ~ZDictController() override;

    void loadDictionaries(const QStringList& pathList);

    QStringList wordLookup(const QString& word,
                           bool suppressMultiforms = false,
                           int maxLookupWords = defaultMaxLookupWords);
    QString loadArticle(const QString& word, bool addDictionaryName = true);

    void setMaxLookupWords(int maxLookupWords);

    QStringList getLoadedDictionaries() const;

Q_SIGNALS:

};

}

#endif // ZDICTCONTROLLER_H
