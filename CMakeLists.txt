cmake_minimum_required(VERSION 3.1.0)

####### Global parameters #############################

project(zdict)
#set(ZDICT ${CMAKE_PROJECT_NAME})

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_MIN_VERSION 5.14)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Xml
)

####### Main project sources ###########################

set(SOURCES
    internal/zdictconversions.cpp
    internal/zdictcompress.cpp
    internal/zdictionary.cpp
    internal/zstardictdictionary.cpp
    zdictcontroller.cpp
)

set(LIBS
    -lz -ltbb
)

####### Main project executable ########################

add_library(zdict
     ${SOURCES}
)

target_link_libraries(zdict
    Qt5::Core
    Qt5::Xml
)
