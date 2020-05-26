INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/zdictcontroller.cpp \
    $$PWD/internal/zdictcompress.cpp \
    $$PWD/internal/zdictionary.cpp \
    $$PWD/internal/zstardictdictionary.cpp

HEADERS += \
    $$PWD/zdictcontroller.h \
    $$PWD/internal/zdictcompress.h \
    $$PWD/internal/zdictionary.h \
    $$PWD/internal/zstardictdictionary.h

LIBS += -lz -ltbb
