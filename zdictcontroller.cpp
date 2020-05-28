#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QString>

#include <algorithm>
#include <execution>

#include "zdictcontroller.h"
#include "internal/zdictionary.h"
#include "internal/zdictcompress.h"
#include "internal/zstardictdictionary.h"

#include <QDebug>

namespace ZDict {

void ZDictController::setMaxLookupWords(int maxLookupWords)
{
    m_maxLookupWords = maxLookupWords;
}

ZDictController::ZDictController(QObject *parent)
    : QObject(parent)
{

}

ZDictController::~ZDictController() = default;

void ZDictController::loadDictionaries(const QStringList &pathList)
{
    QStringList files;

    for (const auto &path : pathList) {
        QDirIterator it(path,QDir::Files | QDir::Readable,QDirIterator::Subdirectories);
        while (it.hasNext())
            files.append(it.next());
    }

    QThread* callerThread = thread();

    std::for_each(std::execution::par,files.constBegin(),files.constEnd(),
            [this,callerThread](const QString& filename){
        QFileInfo fi(filename);
        if (!fi.exists()) return;

        ZDictionary *d = nullptr;

        if (fi.suffix().compare(QSL("ifo"),Qt::CaseInsensitive) == 0) {
            // StarDict dictionary info file
            auto *dict = new ZStardictDictionary();
            if (!dict->loadIndexes(fi.filePath())) {
                qWarning() << QSL("Failed to load StarDict index file %1").arg(fi.filePath());
                delete dict;
                return;
            }
            d = dict;
        }

        if (d) {
            m_dictsMutex.lock();
            m_dicts << QPointer<ZDictionary>(d);
            m_dictsMutex.unlock();

            qInfo() << QSL("Dictionary loaded: %1 (%2)")
                       .arg(d->getName())
                       .arg(d->getWordCount());

            d->moveToThread(callerThread);
        }
    });

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

QString ZDictController::loadArticle(const QString &word, bool addDictionaryName)
{
    const QRegularExpression rx(QSL("\\s+\\[.*\\]"));
    QString res;
    QString w = word.toLower();
    w.remove(rx);

    for (const auto& dict : qAsConst(m_dicts)) {
        const QString article = dict->loadArticle(w);
        if (!article.isEmpty()) {
            QString hr;
            if (!res.isEmpty())
                hr = QSL("<hr/>");

            res.append(hr);
            if (addDictionaryName)
                res.append(QSL("<h4>%1:</h4>").arg(dict->getName()));
            res.append(article);
        }
    }
    return res;
}

}
