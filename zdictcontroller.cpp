#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QString>
#include <QMutexLocker>

#include <algorithm>
#include <execution>

#include "zdictcontroller.h"
#include "internal/zdictionary.h"
#include "internal/zdictcompress.h"
#include "internal/zstardictdictionary.h"

#include <QDebug>

namespace ZQDict {

void ZDictController::setMaxLookupWords(int maxLookupWords)
{
    m_maxLookupWords = maxLookupWords;
}

void ZDictController::loadDictionary(const QString &baseFile)
{
    QFileInfo fi(baseFile);
    if (!fi.exists()) return;

    if (fi.suffix().compare(QSL("ifo"),Qt::CaseInsensitive) == 0) {
        // StarDict dictionary info file

        auto *dict = new ZStardictDictionary(this);
        if (!dict->loadIndexes(fi.filePath())) {
            qWarning() << QSL("Failed to load StarDict index file %1").arg(fi.filePath());
            delete dict;
            return;
        }
        m_dicts << QPointer<ZDictionary>(dict);
        qInfo() << QSL("Dictionary loaded: %1 (%2)")
                   .arg(dict->getName())
                   .arg(dict->getWordCount());
    }
}

ZDictController::ZDictController(QObject *parent)
    : QObject(parent)
{

}

ZDictController::~ZDictController() = default;

void ZDictController::loadDictionaries(const QStringList &pathList)
{
    for (const auto &path : pathList) {
        QDirIterator it(path,QDir::Files | QDir::Readable,QDirIterator::Subdirectories);
        while (it.hasNext()) {
            loadDictionary(it.next());
        }
    }
    qInfo() << QSL("Dictionaries loading complete, %1 dictionaries loaded.").arg(m_dicts.count());
}

QStringList ZDictController::wordLookup(const QString &word,
                                        const QRegularExpression& filter)
{
    QStringList res;

    // Multithreaded word search - one thread per dictionary
    QMutex resMutex;
    std::for_each(std::execution::par,m_dicts.constBegin(),m_dicts.constEnd(),
                  [&res,&resMutex,word,filter](const QPointer<ZDictionary> & ptr){
        const QStringList sl = ptr->wordLookup(word.toLower(),filter);
        resMutex.lock();
        res.append(sl);
        resMutex.unlock();
    });

    // parallel sort and duplicates removing
    std::sort(std::execution::par,res.begin(),res.end());
    res.erase(std::unique(std::execution::par,res.begin(),res.end()),res.end());

    // parallel cutting first n words for result
    QStringList out;
    int nelems = qMin(m_maxLookupWords,res.count());
    out.reserve(nelems);
    std::copy_n(std::execution::par,res.begin(),nelems,std::back_inserter(out));
    return out;
}

QString ZDictController::loadArticle(const QString &word)
{
    QString res;
    for (const auto& dict : qAsConst(m_dicts)) {
        const QString article = dict->loadArticle(word.toLower());
        if (!article.isEmpty()) {
            if (!res.isEmpty())
                res.append(QSL("\n<br>\n"));
            res.append(article);
        }
    }
    return res;
}

}
