/*
 * Parts of this file taken from:
 * - GoldenDict. Licensed under GPLv3 or later, see the LICENSE file.
 *   (c) 2008-2011 Konstantin Isakov <ikm@users.berlios.de>
 */

#include <QDomDocument>
#include "zdictconversions.h"
#include "zdictionary.h"
#include <QDebug>

QString ZDictConversions::htmlPreformat(const QString & str)
{
    QString result = str.toHtmlEscaped();
    result.replace('\t',QSL("&emsp;"));
    result.replace('\n',QSL("<br/>"));
    result.remove('\r');

    return result;
}

QString ZDictConversions::xdxf2Html(const QString& in)
{
    static const QHash<QString,QString> articleStyles = {
        { QSL("example"),   QSL("color:#808080;") },
        { QSL("key"),       QSL("font-weight:bold;") },
        { QSL("abbrev"),    QSL("font-style:italic;color:#2E8B57;") },
        { QSL("editorial"), QSL("font-style:italic;color:#483D8B;") },
        { QSL("dtrn"),      QSL("font-weight:bold;color:#400000;") },
        { QSL("tr"),        QSL("font-weight:bold;") }
    };

    QString inConverted = in;
    inConverted.replace('\n',QSL("<br/>"));

    QDomDocument dd;

    QString errorStr;
    int errorLine, errorColumn;
    if (!dd.setContent(QSL("<div>%1</div>").arg(inConverted).toUtf8(), false, &errorStr, &errorLine, &errorColumn  ) )
    {
        qWarning() << "Xdxf2html error, xml parse failed: " << errorStr << " at "
                   << errorLine << errorColumn;
        qWarning() << "The input was: " << inConverted;
        return in;
    }

    QDomNodeList nodes = dd.elementsByTagName( QSL("ex") ); // Example
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("span") );
        QString style = articleStyles.value(QSL("example"));
        if (!style.isEmpty())
            el.setAttribute(QSL("style"), style);
    }

    nodes = dd.elementsByTagName( QSL("k") ); // Key
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("span") );
        QString style = articleStyles.value(QSL("key"));
        if (!style.isEmpty())
            el.setAttribute(QSL("style"), style);
    }

    nodes = dd.elementsByTagName( QSL("kref") ); // Reference to another word
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("a") );
        el.setAttribute( QSL("href"), QSL( "bword:" ) + el.text() );
    }

    nodes = dd.elementsByTagName( QSL("abr") ); // Abbreviation
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("span") );
        QString style = articleStyles.value(QSL("abbrev"));
        if (!style.isEmpty())
            el.setAttribute(QSL("style"), style);
    }

    nodes = dd.elementsByTagName( QSL("dtrn") ); // Direct translation
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("span") );
        QString style = articleStyles.value(QSL("dtrn"));
        if (!style.isEmpty())
            el.setAttribute(QSL("style"), style);
    }

    nodes = dd.elementsByTagName( QSL("c") ); // Color
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("font") );
        if ( el.hasAttribute( QSL("c") ) ) {
            el.setAttribute( QSL("color"), el.attribute( QSL("c") ) );
            el.removeAttribute( QSL("c") );
        }
    }

    nodes = dd.elementsByTagName( QSL("co") ); // Editorial comment
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName( QSL("span") );
        QString style = articleStyles.value(QSL("editorial"));
        if (!style.isEmpty())
            el.setAttribute(QSL("style"), style);
    }

    nodes = dd.elementsByTagName( QSL("tr") ); // Transcription
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(QSL("span"));
        QString style = articleStyles.value(QSL("tr"));
        if (!style.isEmpty())
            el.setAttribute(QSL("style"), style);
    }

    nodes = dd.elementsByTagName( QSL("rref") ); // Resource reference
    while (nodes.size()>0) {
        QDomElement el = nodes.at(0).toElement();
        el.setTagName(QSL("span"));
        el.setAttribute(QSL("style"), QSL("display:none;") );
    }

    QString res = dd.toString();
    return res;
}
