#include <algorithm>
#include <execution>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QString>
#include <QThread>
#include <QCoreApplication>

#include "zdictcontroller.h"
#include "internal/zdictionary.h"
#include "internal/zdictcompress.h"
#include "internal/zstardictdictionary.h"

#include <QDebug>

namespace ZDict {

ZDictController::ZDictController(QObject *parent)
    : QObject(parent)
{
}

ZDictController::~ZDictController() = default;

void ZDictController::loadDictionaries(const QStringList &pathList)
{
    m_dicts.clear();

    QThread* th = QThread::create([this,pathList]{

        QStringList files;

        for (const auto &path : pathList) {
            QDirIterator it(path,QDir::Files | QDir::Readable,QDirIterator::Subdirectories);
            while (it.hasNext())
                files.append(it.next());
        }

        QAtomicInteger<int> wordCount;
        std::for_each(std::execution::par,files.constBegin(),files.constEnd(),
                      [this,&wordCount](const QString& filename){
            if (QCoreApplication::closingDown()) return;

            QFileInfo fi(filename);
            if (!fi.exists()) return;

            ZDictionary *d = nullptr;

            if (fi.suffix().compare(ZDQSL("ifo"),Qt::CaseInsensitive) == 0) {
                // StarDict dictionary info file
                auto *dict = new ZStardictDictionary();
                if (!dict->loadIndexes(fi.filePath())) {
                    qWarning() << ZDQSL("Failed to load StarDict index file %1").arg(fi.filePath());
                    delete dict;
                    return;
                }
                d = dict;
            }

            if (d) {
                if (QCoreApplication::closingDown()) return;

                m_dictsMutex.lock();
                m_dicts.append(QSharedPointer<ZDictionary>(d));
                m_dictsMutex.unlock();

                int wc = d->getWordCount();
                wordCount.fetchAndAddRelease(wc);
                qInfo() << ZDQSL("Dictionary loaded: %1 (%2)")
                           .arg(d->getName())
                           .arg(wc);
            }
        });

        int dictsCount = m_dicts.count();
        qInfo() << ZDQSL("Dictionaries loading complete, %1 dictionaries loaded.").arg(dictsCount);
        Q_EMIT dictionariesLoaded(ZDQSL("Loaded %1 dictionaries (%2 words).")
                                  .arg(dictsCount).arg(wordCount.loadAcquire()));
        m_loaded.storeRelease(true);
    });

    connect(th,&QThread::finished,th,&QThread::deleteLater);
    th->setObjectName(ZDQSL("ZDICT_startup"));
    th->start();
}

QStringList ZDictController::wordLookup(const QString &word,
                                        bool suppressMultiforms,
                                        int maxLookupWords)
{
    QStringList res;
    if (!m_loaded.loadAcquire()) return res;
    if (word.isEmpty()) return res;

    QString w = word.toLower();
    w.remove(QRegularExpression(ZDQSL("\\s+.*"),QRegularExpression::UseUnicodePropertiesOption));
    w.remove(QRegularExpression(ZDQSL("\\W+"),QRegularExpression::UseUnicodePropertiesOption));

    // Multithreaded word search - one thread per dictionary
    QMutex resMutex;
    std::for_each(std::execution::par,m_dicts.constBegin(),m_dicts.constEnd(),
                  [&res,&resMutex,w,maxLookupWords,suppressMultiforms](const QSharedPointer<ZDictionary> & ptr){
        ptr->resetStopRequest();
        const QStringList sl = ptr->wordLookup(w,suppressMultiforms,maxLookupWords);
        resMutex.lock();
        res.append(sl);
        resMutex.unlock();
    });

    // parallel sort and duplicates removing
    std::sort(std::execution::par,res.begin(),res.end());
    res.erase(std::unique(std::execution::par,res.begin(),res.end()),res.end());

    // parallel cutting first n words for result
    QStringList out;
    int nelems = qMin(maxLookupWords,res.count());
    out.reserve(nelems);
    std::copy_n(std::execution::par,res.begin(),nelems,std::back_inserter(out));
    return out;
}

void ZDictController::wordLookupAsync(const QString &word, bool suppressMultiforms, int maxLookupWords)
{
    QThread *th = QThread::create([this,word,suppressMultiforms,maxLookupWords]{
        QStringList res = wordLookup(word,suppressMultiforms,maxLookupWords);
        Q_EMIT wordListComplete(res);
    });
    connect(th,&QThread::finished,th,&QThread::deleteLater);
    th->setObjectName(ZDQSL("ZDICT_lookup"));
    th->start();
}

QString ZDictController::loadArticle(const QString &word, bool addDictionaryName)
{
    const QRegularExpression rx(ZDQSL("\\s+\\[.*\\]"));

    QString res;
    if (!m_loaded.loadAcquire()) return res;

    QString w = word.toLower();
    w.remove(rx);

    for (const auto& dict : qAsConst(m_dicts)) {
        dict->resetStopRequest();
        const QString article = dict->loadArticle(w);
        if (!article.isEmpty()) {
            QString hr;
            if (!res.isEmpty())
                hr = ZDQSL("<hr/>");

            res.append(hr);
            if (addDictionaryName)
                res.append(ZDQSL("<h4>%1:</h4>").arg(dict->getName()));
            res.append(article);
        }
    }
    return res;
}

void ZDictController::loadArticleAsync(const QString &word, bool addDictionaryName)
{
    QThread *th = QThread::create([this,word,addDictionaryName]{
        QString res = loadArticle(word,addDictionaryName);
        Q_EMIT articleComplete(res);
    });
    connect(th,&QThread::finished,th,&QThread::deleteLater);
    th->setObjectName(ZDQSL("ZDICT_article"));
    th->start();
}

void ZDictController::cancelActiveWork()
{
    for (const auto &dict : qAsConst(m_dicts)) {
        dict->stopRequest();
    }
}

QStringList ZDictController::getLoadedDictionaries() const
{
    QStringList res;
    if (!m_loaded.loadAcquire()) return res;

    res.reserve(m_dicts.count());
    for (const auto & dict : qAsConst(m_dicts))
        res.append(ZDQSL("%1 (%2)").arg(dict->getName()).arg(dict->getWordCount()));

    return res;
}

}
