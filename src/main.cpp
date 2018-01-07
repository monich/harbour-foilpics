/*
 * Copyright (C) 2017-2018 Jolla Ltd.
 * Copyright (C) 2017-2018 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the name of Jolla Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FoilPicsBusyState.h"
#include "FoilPicsDefs.h"
#include "FoilPicsGalleryPlugin.h"
#include "FoilPicsHints.h"
#include "FoilPicsModelWatch.h"
#include "FoilPicsModel.h"
#include "FoilPicsSelection.h"
#include "FoilPicsSelectionState.h"
#include "FoilPicsThumbnailerPlugin.h"
#include "FoilPicsFileUtil.h"

#include "HarbourDebug.h"
#include "HarbourLib.h"
#include "HarbourTransferMethodsModel.h"

#include <sailfishapp.h>
#include <gutil_log.h>

#include <QGuiApplication>
#include <QtQuick>

static void register_types(const char* uri, int v1 = 1, int v2 = 0)
{
    HarbourLib::registerTypes(uri, v1, v2);
    qmlRegisterType<FoilPicsBusyState>(uri, v1, v2, "FoilPicsBusyState");
    qmlRegisterType<FoilPicsHints>(uri, v1, v2, "FoilPicsHints");
    qmlRegisterType<FoilPicsModel>(uri, v1, v2, "FoilPicsModel");
    qmlRegisterType<FoilPicsModelWatch>(uri, v1, v2, "FoilPicsModelWatch");
    qmlRegisterType<FoilPicsSelection>(uri, v1, v2, "FoilPicsSelection");
    qmlRegisterType<FoilPicsSelectionState>(uri, v1, v2, "FoilPicsSelectionState");
}

int main(int argc, char *argv[])
{
    QGuiApplication* app = SailfishApp::application(argc, argv);
    app->setApplicationName(FOILPICS_APP_NAME);

    register_types(FOILPICS_QML_IMPORT, 1, 0);
    HarbourTransferMethodInfo::registerTypes();

    // Load translations
    QLocale locale;
    QTranslator* tr = new QTranslator(app);
#ifdef OPENREPOS
    // OpenRepos build has settings applet
    const QString transDir("/usr/share/translations");
#else
    const QString transDir = SailfishApp::pathTo("translations").toLocalFile();
#endif
    const QString transFile(FOILPICS_APP_NAME);
    if (tr->load(locale, transFile, "-", transDir) ||
        tr->load(transFile, transDir)) {
        app->installTranslator(tr);
    } else {
        HDEBUG("Failed to load translator for" << locale);
        delete tr;
    }
    tr = new QTranslator(app);
    if (HarbourTransferMethodsModel::loadTranslations(tr, locale) ||
        HarbourTransferMethodsModel::loadTranslations(tr, QLocale("en_GB"))) {
        app->installTranslator(tr);
    } else {
        delete tr;
    }

    gutil_log_timestamp = FALSE;
    gutil_log_func = gutil_log_stdout;
    gutil_log_default.name = FOILPICS_APP_NAME;
#ifdef DEBUG
    gutil_log_default.level = GLOG_LEVEL_VERBOSE;
#endif

    // Create ans show the view
    QQuickView* view = SailfishApp::createView();
    QQmlContext* context = view->rootContext();

    // Re-register some types
    FoilPicsGalleryPlugin::registerTypes(context->engine(),
        FOILPICS_GALLERY_QML_IMPORT, 1, 0);
    FoilPicsThumbnailerPlugin::registerTypes(context->engine(),
        FOILPICS_THUMBNAILER_QML_IMPORT, 1, 0);

    // Initialize the view and the global properties
    view->setTitle(qtTrId("foilpics-app_name"));
    context->setContextProperty("MaximumHintCount", 1);
    context->setContextProperty("FileUtil", new FoilPicsFileUtil(app));
    //context->setContextProperty("AppSettings", new AppSettings(app));

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();

    int ret = app->exec();

    delete view;
    delete app;
    return ret;
}
