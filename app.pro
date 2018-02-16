NAME = foilpics

openrepos {
    PREFIX = openrepos
    DEFINES += OPENREPOS
} else {
    PREFIX = harbour
}

TARGET = $${PREFIX}-$${NAME}
CONFIG += sailfishapp link_pkgconfig
PKGCONFIG += sailfishapp mlite5 glib-2.0 gobject-2.0 libcrypto
QT += qml quick dbus

QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-psabi
QMAKE_CFLAGS += -Wno-unused-parameter

app_settings {
    # This path is hardcoded in jolla-settings
    TRANSLATIONS_PATH = /usr/share/translations
} else {
    TRANSLATIONS_PATH = /usr/share/$${TARGET}/translations
}

CONFIG(debug, debug|release) {
  DEFINES += DEBUG HARBOUR_DEBUG
}

# Directories
HARBOUR_LIB_REL = harbour-lib
HARBOUR_LIB_DIR = $${_PRO_FILE_PWD_}/$${HARBOUR_LIB_REL}

LIBGLIBUTIL_DIR = $${_PRO_FILE_PWD_}/libglibutil
LIBGLIBUTIL_INCLUDE = $${LIBGLIBUTIL_DIR}/include

FOIL_DIR = $${_PRO_FILE_PWD_}/foil
LIBFOIL_DIR = $${FOIL_DIR}/libfoil
LIBFOIL_INCLUDE = $${LIBFOIL_DIR}/include
LIBFOIL_SRC = $${LIBFOIL_DIR}/src

LIBFOILMSG_DIR = $${FOIL_DIR}/libfoilmsg
LIBFOILMSG_INCLUDE = $${LIBFOILMSG_DIR}/include
LIBFOILMSG_SRC = $${LIBFOILMSG_DIR}/src

# Libraries
HARBOUR_LIB = $${OUT_PWD}/$${HARBOUR_LIB_REL}/libharbour-lib.a

PRE_TARGETDEPS += \
    $$HARBOUR_LIB

LIBS += $$HARBOUR_LIB -ldl

OTHER_FILES += \
    *.desktop \
    qml/*.qml \
    qml/images/*.svg \
    icons/*.svg \
    translations/*.ts

INCLUDEPATH += \
    src \
    $${LIBFOIL_SRC} \
    $${LIBFOIL_INCLUDE} \
    $${LIBFOILMSG_INCLUDE} \
    $${LIBGLIBUTIL_INCLUDE} \
    $${HARBOUR_LIB_DIR}/include

HEADERS += \
    src/FoilPicsBusyState.h \
    src/FoilPicsDefs.h \
    src/FoilPicsFileUtil.h \
    src/FoilPicsGalleryPlugin.h \
    src/FoilPicsGroupModel.h \
    src/FoilPicsHints.h \
    src/FoilPicsImageProvider.h \
    src/FoilPicsImageRequest.h \
    src/FoilPicsModel.h \
    src/FoilPicsModelWatch.h \
    src/FoilPicsRole.h \
    src/FoilPicsSelection.h \
    src/FoilPicsSelectionState.h \
    src/FoilPicsTask.h \
    src/FoilPicsThumbnailerPlugin.h \
    src/FoilPicsThumbnailProvider.h

SOURCES += \
    src/FoilPicsBusyState.cpp \
    src/FoilPicsFileUtil.cpp \
    src/FoilPicsGalleryPlugin.cpp \
    src/FoilPicsGroupModel.cpp \
    src/FoilPicsHints.cpp \
    src/FoilPicsImageProvider.cpp \
    src/FoilPicsImageRequest.cpp \
    src/FoilPicsModel.cpp \
    src/FoilPicsModelWatch.cpp \
    src/FoilPicsRole.cpp \
    src/FoilPicsSelection.cpp \
    src/FoilPicsSelectionState.cpp \
    src/FoilPicsTask.cpp \
    src/FoilPicsThumbnailerPlugin.cpp \
    src/FoilPicsThumbnailProvider.cpp \
    src/main.cpp

SOURCES += \
    $${LIBFOIL_SRC}/*.c \
    $${LIBFOIL_SRC}/openssl/*.c \
    $${LIBFOILMSG_SRC}/*.c \
    $${LIBGLIBUTIL_DIR}/src/*.c

HEADERS += \
    $${LIBFOIL_INCLUDE}/*.h \
    $${LIBFOILMSG_INCLUDE}/*.h \
    $${LIBGLIBUTIL_INCLUDE}/*.h

# Icons
ICON_SIZES = 86 108 128 256
for(s, ICON_SIZES) {
    icon_target = icon$${s}
    icon_dir = icons/$${s}x$${s}
    $${icon_target}.files = $${icon_dir}/$${TARGET}.png
    $${icon_target}.path = /usr/share/icons/hicolor/$${s}x$${s}/apps
    equals(PREFIX, "openrepos") {
        $${icon_target}.extra = cp $${icon_dir}/harbour-$${NAME}.png $$eval($${icon_target}.files)
        $${icon_target}.CONFIG += no_check_exist
    }
    INSTALLS += $${icon_target}
}

# Desktop file
equals(PREFIX, "openrepos") {
    desktop.extra = sed s/harbour/openrepos/g harbour-$${NAME}.desktop > $${TARGET}.desktop
    desktop.CONFIG += no_check_exist
}

# Translations
TRANSLATION_SOURCES = \
  $${_PRO_FILE_PWD_}/qml

defineTest(addTrFile) {
    in = $${_PRO_FILE_PWD_}/translations/harbour-$$1
    out = $${OUT_PWD}/translations/$${PREFIX}-$$1

    s = $$replace(1,-,_)
    lupdate_target = lupdate_$$s
    lrelease_target = lrelease_$$s

    $${lupdate_target}.commands = lupdate -noobsolete $${TRANSLATION_SOURCES} -ts \"$${in}.ts\" && \
        mkdir -p \"$${OUT_PWD}/translations\" &&  [ \"$${in}.ts\" != \"$${out}.ts\" ] && \
        cp -af \"$${in}.ts\" \"$${out}.ts\" || :

    $${lrelease_target}.target = $${out}.qm
    $${lrelease_target}.depends = $${lupdate_target}
    $${lrelease_target}.commands = lrelease -idbased \"$${out}.ts\"

    QMAKE_EXTRA_TARGETS += $${lrelease_target} $${lupdate_target}
    PRE_TARGETDEPS += $${out}.qm
    qm.files += $${out}.qm

    export($${lupdate_target}.commands)
    export($${lrelease_target}.target)
    export($${lrelease_target}.depends)
    export($${lrelease_target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(PRE_TARGETDEPS)
    export(qm.files)
}

LANGUAGES = de es fr hu nl pl ru sv

addTrFile($${NAME})
for(l, LANGUAGES) {
    addTrFile($${NAME}-$$l)
}

qm.path = $$TRANSLATIONS_PATH
qm.CONFIG += no_check_exist
INSTALLS += qm
