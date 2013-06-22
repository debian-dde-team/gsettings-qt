TEMPLATE = app
QT += qml testlib
CONFIG += qmltestcase
TARGET = test
IMPORTPATH = $$PWD/..
SOURCES = test.cpp

schema.target = gschemas.compiled
schema.commands = glib-compile-schemas $$PWD
schema.depends = com.canonical.gsettings.test.gschema.xml
QMAKE_EXTRA_TARGETS += schema
PRE_TARGETDEPS = gschemas.compiled

# qmake prepends this to the command line executed by `make check`
check.commands = GSETTINGS_BACKEND=memory GSETTINGS_SCHEMA_DIR=$$PWD