#ifndef ZDICTCONVERSIONS_H
#define ZDICTCONVERSIONS_H

#include <QString>

class ZDictConversions
{
public:
    ZDictConversions() {};

    static QString htmlPreformat(const QString &str);
    static QString xdxf2Html(const QString &in);
};

#endif // ZDICTCONVERSIONS_H
