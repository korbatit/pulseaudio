TARGET  = pulseaudio

TEMPLATE = lib
CONFIG += qt plugin

QT     += multimedia

LIBS+=-L/usr/lib/i386-linux-gnu -lpulse-simple

HEADERS += pulseaudio.h
SOURCES += main.cpp \
           pulseaudio.cpp
