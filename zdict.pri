INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/internal/zdictconversions.cpp \
    $$PWD/zdictcontroller.cpp \
    $$PWD/internal/zdictcompress.cpp \
    $$PWD/internal/zstardictdictionary.cpp

HEADERS += \
    $$PWD/internal/zdictconversions.h \
    $$PWD/zdictcontroller.h \
    $$PWD/internal/zdictcompress.h \
    $$PWD/internal/zdictionary.h \
    $$PWD/internal/zstardictdictionary.h

LIBS += -lz -ltbb
