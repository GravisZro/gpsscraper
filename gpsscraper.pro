TEMPLATE = app
CONFIG += console
CONFIG += c++2a
CONFIG += strict_c++
#CONFIG += exceptions_off
CONFIG += rtti_off

CONFIG -= app_bundle
CONFIG -= qt

#clang:CONFIG += win32

#DEFINES += DATABASE_TOLERANT

#QMAKE_CXXFLAGS_DEBUG += -DDEBUG_BUILD
QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_CXXFLAGS_RELEASE += -Os
#QMAKE_CXXFLAGS += -Os
#QMAKE_CXXFLAGS += -ffreestanding
QMAKE_CXXFLAGS += -fno-threadsafe-statics
#linux:QMAKE_LFLAGS += -L/usr/lib/x86_64-linux-musl
#linux:QMAKE_LFLAGS += -lc

#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++

CONFIG += link_pkgconfig
PKGCONFIG += libcurl
PKGCONFIG += sqlite3


SOURCES += \
        dbinterface.cpp \
        main.cpp \
        scrapers/chargehub.cpp \
        scrapers/electrifyamerica.cpp \
        scrapers/eptix.cpp \
        scrapers/evgo.cpp \
        scrapers/scraper_types.cpp \
        scrapers/utilities.cpp \
        scrapers/scraper_base.cpp \
        shortjson/shortjson_strict.cpp \
        simplified/simple_sqlite.cpp \
        tinf/src/adler32.c \
        tinf/src/crc32.c \
        tinf/src/tinfgzip.c \
        tinf/src/tinflate.c \
        tinf/src/tinfzlib.c

HEADERS += \
  dbinterface.h \
  scrapers/chargehub.h \
  scrapers/electrifyamerica.h \
  scrapers/eptix.h \
  scrapers/evgo.h \
  scrapers/scraper_types.h \
  scrapers/utilities.h \
  scrapers/scraper_base.h \
  shortjson/shortjson.h \
  simplified/simple_curl.h \
  simplified/simple_sqlite.h \
  tinf/src/tinf.h
