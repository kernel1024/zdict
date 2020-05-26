#ifndef ZDICTCONTROLLER_H
#define ZDICTCONTROLLER_H

#include <QObject>
#include <QMap>
#include <QPointer>

#include "internal/zdictionary.h"

namespace ZQDict {

class ZDictController : public QObject
{
    Q_OBJECT
private:
    QVector<QPointer<ZDictionary> > m_dicts;
    int m_maxLookupWords { 10000 };

    void loadDictionary(const QString& baseFile);

public:
    explicit ZDictController(QObject *parent = nullptr);
    ~ZDictController() override;

    void loadDictionaries(const QStringList& pathList);

    QStringList wordLookup(const QString& word,
                           const QRegularExpression& filter = QRegularExpression());
    QString loadArticle(const QString& word);

    void setMaxLookupWords(int maxLookupWords);

signals:

};

}

#endif // ZDICTCONTROLLER_H
