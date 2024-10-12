PREFIX = harbour
NAME = foilpics

openrepos {
    DEFINES += OPENREPOS
    CONFIG += app_settings
}

TARGET = $${PREFIX}-$${NAME}
CONFIG += sailfishapp link_pkgconfig
PKGCONFIG += sailfishapp keepalive mlite5 glib-2.0 gobject-2.0
QT += qml quick dbus

QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-psabi
QMAKE_CFLAGS += -Wno-unused-parameter

LIBS += -ldl

app_settings {
    # This path is hardcoded in jolla-settings
    TRANSLATIONS_PATH = /usr/share/translations
} else {
    TRANSLATIONS_PATH = /usr/share/$${TARGET}/translations
}

CONFIG(debug, debug|release) {
  DEFINES += DEBUG HARBOUR_DEBUG
}

equals(QT_ARCH, arm64){
    message(Linking with OpenSSL)
    PKGCONFIG += libcrypto
}

# Fix libfoil compilation warnings:
DEFINES += GLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32
DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_MAX_ALLOWED

# Directories
FOIL_UI_REL = foil-ui
FOIL_UI_DIR = $${_PRO_FILE_PWD_}/$${FOIL_UI_REL}
FOIL_UI_QML = $${FOIL_UI_DIR}

HARBOUR_LIB_REL = harbour-lib
HARBOUR_LIB_DIR = $${_PRO_FILE_PWD_}/$${HARBOUR_LIB_REL}
HARBOUR_LIB_INCLUDE = $${HARBOUR_LIB_DIR}/include
HARBOUR_LIB_SRC = $${HARBOUR_LIB_DIR}/src
HARBOUR_LIB_QML = $${HARBOUR_LIB_DIR}/qml

LIBGLIBUTIL_DIR = $${_PRO_FILE_PWD_}/libglibutil
LIBGLIBUTIL_INCLUDE = $${LIBGLIBUTIL_DIR}/include

FOIL_DIR = $${_PRO_FILE_PWD_}/foil
LIBFOIL_DIR = $${FOIL_DIR}/libfoil
LIBFOIL_INCLUDE = $${LIBFOIL_DIR}/include
LIBFOIL_SRC = $${LIBFOIL_DIR}/src

LIBFOILMSG_DIR = $${FOIL_DIR}/libfoilmsg
LIBFOILMSG_INCLUDE = $${LIBFOILMSG_DIR}/include
LIBFOILMSG_SRC = $${LIBFOILMSG_DIR}/src

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
    $${HARBOUR_LIB_INCLUDE}

HEADERS += \
    src/FoilPics.h \
    src/FoilPicsBusyState.h \
    src/FoilPicsDefs.h \
    src/FoilPicsGroupModel.h \
    src/FoilPicsHints.h \
    src/FoilPicsImageProvider.h \
    src/FoilPicsImageRequest.h \
    src/FoilPicsModel.h \
    src/FoilPicsModelWatch.h \
    src/FoilPicsRole.h \
    src/FoilPicsSelection.h \
    src/FoilPicsSelectionState.h \
    src/FoilPicsSettings.h \
    src/FoilPicsThumbnailProvider.h

SOURCES += \
    src/FoilPics.cpp \
    src/FoilPicsBusyState.cpp \
    src/FoilPicsGroupModel.cpp \
    src/FoilPicsHints.cpp \
    src/FoilPicsImageProvider.cpp \
    src/FoilPicsImageRequest.cpp \
    src/FoilPicsModel.cpp \
    src/FoilPicsModelWatch.cpp \
    src/FoilPicsRole.cpp \
    src/FoilPicsSelection.cpp \
    src/FoilPicsSelectionState.cpp \
    src/FoilPicsSettings.cpp \
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

# foil-ui

FOIL_UI_COMPONENTS = \
    $${FOIL_UI_QML}/FoilUiAppsWarning.qml \
    $${FOIL_UI_QML}/FoilUiChangePasswordPage.qml \
    $${FOIL_UI_QML}/FoilUiConfirmPasswordDialog.qml \
    $${FOIL_UI_QML}/FoilUiEnterPasswordView.qml \
    $${FOIL_UI_QML}/FoilUiGenerateKeyPage.qml \
    $${FOIL_UI_QML}/FoilUiGenerateKeyView.qml \
    $${FOIL_UI_QML}/FoilUiGenerateKeyWarning.qml \
    $${FOIL_UI_QML}/FoilUiGeneratingKeyView.qml

FOIL_UI_IMAGES = $${FOIL_UI_QML}/images/*.svg

OTHER_FILES += $${FOIL_UI_COMPONENTS}
foil_ui_components.files = $${FOIL_UI_COMPONENTS}
foil_ui_components.path = /usr/share/$${TARGET}/qml/foil-ui
INSTALLS += foil_ui_components

OTHER_FILES += $${FOIL_UI_IMAGES}
foil_ui_images.files = $${FOIL_UI_IMAGES}
foil_ui_images.path = /usr/share/$${TARGET}/qml/foil-ui/images
INSTALLS += foil_ui_images

# harbour-lib

HEADERS += \
    $${HARBOUR_LIB_INCLUDE}/HarbourBase45.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourDebug.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourProcessState.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSystemInfo.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSystemState.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourTask.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourTransferMethodInfo.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourTransferMethodsModel.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourWakeupTimer.h \
    $${HARBOUR_LIB_SRC}/HarbourMce.h

SOURCES += \
    $${HARBOUR_LIB_SRC}/HarbourBase45.cpp \
    $${HARBOUR_LIB_SRC}/HarbourMce.cpp \
    $${HARBOUR_LIB_SRC}/HarbourProcessState.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSystemInfo.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSystemState.cpp \
    $${HARBOUR_LIB_SRC}/HarbourTask.cpp \
    $${HARBOUR_LIB_SRC}/HarbourTransferMethodInfo.cpp \
    $${HARBOUR_LIB_SRC}/HarbourTransferMethodsModel.cpp \
    $${HARBOUR_LIB_SRC}/HarbourWakeupTimer.cpp

HARBOUR_QML_COMPONENTS = \
    $${HARBOUR_LIB_QML}/HarbourBadge.qml \
    $${HARBOUR_LIB_QML}/HarbourFitLabel.qml \
    $${HARBOUR_LIB_QML}/HarbourHorizontalSwipeHint.qml \
    $${HARBOUR_LIB_QML}/HarbourHighlightIcon.qml \
    $${HARBOUR_LIB_QML}/HarbourIconTextButton.qml \
    $${HARBOUR_LIB_QML}/HarbourPasswordInputField.qml \
    $${HARBOUR_LIB_QML}/HarbourShakeAnimation.qml \
    $${HARBOUR_LIB_QML}/HarbourShareMethodList.qml

OTHER_FILES += $${HARBOUR_QML_COMPONENTS}

qml_components.files = $${HARBOUR_QML_COMPONENTS}
qml_components.path = /usr/share/$${TARGET}/qml/harbour
INSTALLS += qml_components

# openssl
!equals(QT_ARCH, arm64){
SOURCES += \
    $${HARBOUR_LIB_SRC}/libcrypto.c
}

# Icons
ICON_SIZES = 86 108 128 172 256
for(s, ICON_SIZES) {
    icon_target = icon$${s}
    icon_dir = icons/$${s}x$${s}
    $${icon_target}.files = $${icon_dir}/$${TARGET}.png
    $${icon_target}.path = /usr/share/icons/hicolor/$${s}x$${s}/apps
    INSTALLS += $${icon_target}
}

# Settings
app_settings {
    settings_json.files = settings/$${TARGET}.json
    settings_json.path = /usr/share/jolla-settings/entries/
    settings_qml.files = settings/*.qml
    settings_qml.path = /usr/share/$${TARGET}/qml/settings/
    INSTALLS += settings_qml settings_json
}
OTHER_FILES += \
    settings/*.qml \
    settings/*.json

# Translations
TRANSLATION_IDBASED=-idbased
TRANSLATION_SOURCES = \
    $${_PRO_FILE_PWD_}/qml \
    $${_PRO_FILE_PWD_}/settings

defineTest(addTrFile) {
    rel = translations/$${1}
    OTHER_FILES += $${rel}.ts
    export(OTHER_FILES)

    in = $${_PRO_FILE_PWD_}/$$rel
    out = $${OUT_PWD}/$$rel

    s = $$replace(1,-,_)
    lupdate_target = lupdate_$$s
    qm_target = qm_$$s

    $${lupdate_target}.commands = lupdate -noobsolete -locations none $${TRANSLATION_SOURCES} -ts \"$${in}.ts\" && \
        mkdir -p \"$${OUT_PWD}/translations\" &&  [ \"$${in}.ts\" != \"$${out}.ts\" ] && \
        cp -af \"$${in}.ts\" \"$${out}.ts\" || :

    $${qm_target}.path = $$TRANSLATIONS_PATH
    $${qm_target}.depends = $${lupdate_target}
    $${qm_target}.commands = lrelease $$TRANSLATION_IDBASED \"$${out}.ts\" && \
        $(INSTALL_FILE) \"$${out}.qm\" $(INSTALL_ROOT)$${TRANSLATIONS_PATH}/

    QMAKE_EXTRA_TARGETS += $${lupdate_target} $${qm_target}
    INSTALLS += $${qm_target}

    export($${lupdate_target}.commands)
    export($${qm_target}.path)
    export($${qm_target}.depends)
    export($${qm_target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(INSTALLS)
}

LANGUAGES = de es fr hu nl pl ru sv zh_CN

addTrFile($${TARGET})
for(l, LANGUAGES) {
    addTrFile($${TARGET}-$$l)
}

qm.path = $$TRANSLATIONS_PATH
qm.CONFIG += no_check_exist
INSTALLS += qm
