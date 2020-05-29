/*
 * Parts of this file taken from:
 * - GoldenDict. Licensed under GPLv3 or later, see the LICENSE file.
 *   (c) 2008-2011 Konstantin Isakov <ikm@users.berlios.de>
 */

#include <QDomDocument>
#include <QUrl>
#include "zdictconversions.h"
#include "zdictionary.h"
#include <QDebug>

QString ZDictConversions::htmlPreformat(const QString & str)
{
    QString result = str.toHtmlEscaped();
    result.replace('\t',ZDQSL("&emsp;"));
    result.replace('\n',ZDQSL("<br/>"));
    result.remove('\r');

    return result;
}

QString ZDictConversions::xdxf2Html(const QString& in)
{
    static const QHash<QString,QString> articleStyles = {
        { ZDQSL("example"),   ZDQSL("color:#808080;") },
        { ZDQSL("key"),       ZDQSL("font-weight:bold;") },
        { ZDQSL("abbrev"),    ZDQSL("font-style:italic;color:#2E8B57;") },
        { ZDQSL("editorial"), ZDQSL("font-style:italic;color:#483D8B;") },
        { ZDQSL("dtrn"),      ZDQSL("font-weight:bold;color:#400000;") },
        { ZDQSL("tr"),        ZDQSL("font-weight:bold;") }
    };

    QString inConverted = in;
    inConverted.replace('\n',ZDQSL("<br/>"));

    QDomDocument dd;

    QString errorStr;
    int errorLine = 0;
    int errorColumn = 0;
    if (!dd.setContent(ZDQSL("<div>%1</div>").arg(inConverted).toUtf8(), false, &errorStr, &errorLine, &errorColumn  ) )
    {
        qWarning() << "Xdxf2html error, xml parse failed: " << errorStr << " at "
                   << errorLine << errorColumn;
        qWarning() << "The input was: " << inConverted;
        return in;
    }

    QDomNodeList nodes = dd.elementsByTagName(ZDQSL("ex")); // Example
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        QString style = articleStyles.value(ZDQSL("example"));
        if (!style.isEmpty())
            el.setAttribute(ZDQSL("style"), style);
    }

    nodes = dd.elementsByTagName(ZDQSL("k")); // Key
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        QString style = articleStyles.value(ZDQSL("key"));
        if (!style.isEmpty())
            el.setAttribute(ZDQSL("style"), style);
    }

    nodes = dd.elementsByTagName(ZDQSL("kref")); // Reference to another word
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("a"));
        el.setAttribute(ZDQSL("href"), ZDQSL( "zdict?word=" ) + QUrl::toPercentEncoding(el.text()));
    }

    nodes = dd.elementsByTagName(ZDQSL("abr")); // Abbreviation
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        QString style = articleStyles.value(ZDQSL("abbrev"));
        if (!style.isEmpty())
            el.setAttribute(ZDQSL("style"), style);
    }

    nodes = dd.elementsByTagName(ZDQSL("dtrn")); // Direct translation
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        QString style = articleStyles.value(ZDQSL("dtrn"));
        if (!style.isEmpty())
            el.setAttribute(ZDQSL("style"), style);
    }

    nodes = dd.elementsByTagName(ZDQSL("c")); // Color
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("font"));
        if ( el.hasAttribute(ZDQSL("c"))) {
            el.setAttribute(ZDQSL("color"), el.attribute(ZDQSL("c")));
            el.removeAttribute(ZDQSL("c"));
        }
    }

    nodes = dd.elementsByTagName(ZDQSL("co")); // Editorial comment
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        QString style = articleStyles.value(ZDQSL("editorial"));
        if (!style.isEmpty())
            el.setAttribute(ZDQSL("style"), style);
    }

    nodes = dd.elementsByTagName(ZDQSL("tr")); // Transcription
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        QString style = articleStyles.value(ZDQSL("tr"));
        if (!style.isEmpty())
            el.setAttribute(ZDQSL("style"), style);
    }

    nodes = dd.elementsByTagName(ZDQSL("rref")); // Resource reference
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(ZDQSL("span"));
        el.setAttribute(ZDQSL("style"), ZDQSL("display:none;"));
    }

    QString res = dd.toString();
    return res;
}
