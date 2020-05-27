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
    int m_maxLookupWords { 10000 };

public:
    explicit ZDictController(QObject *parent = nullptr);
    ~ZDictController() override;

    void loadDictionaries(const QStringList& pathList);

    QStringList wordLookup(const QString& word,
                           const QRegularExpression& filter = QRegularExpression());
    QString loadArticle(const QString& word, bool addDictionaryName = true);

    void setMaxLookupWords(int maxLookupWords);

Q_SIGNALS:

};

}

#endif // ZDICTCONTROLLER_H
