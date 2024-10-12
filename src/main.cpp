/*
 * Copyright (C) 2017-2024 Slava Monich <slava@monich.com>
 * Copyright (C) 2017-2022 Jolla Ltd.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "FoilPicsBusyState.h"
#include "FoilPicsDefs.h"
#include "FoilPicsHints.h"
#include "FoilPicsModelWatch.h"
#include "FoilPicsModel.h"
#include "FoilPicsSelection.h"
#include "FoilPicsSelectionState.h"
#include "FoilPicsSettings.h"
#include "FoilPics.h"

#include "HarbourDebug.h"
#include "HarbourProcessState.h"
#include "HarbourSystemInfo.h"
#include "HarbourSystemState.h"
#include "HarbourTransferMethodsModel.h"
#include "HarbourWakeupTimer.h"

#include <sailfishapp.h>
#include <gutil_log.h>

#include <QtGui/QGuiApplication>
#include <QtQuick>

static void register_types(const char* uri, int v1 = 1, int v2 = 0)
{
#define REGISTER_TYPE(uri, v1, v2, Class) \
    qmlRegisterType<Class>(uri, v1, v2, #Class)
#define REGISTER_SINGLETON_TYPE(uri, v1, v2, Class) \
    qmlRegisterSingletonType<Class>(uri, v1, v2, #Class, \
    Class::createSingleton)

    REGISTER_SINGLETON_TYPE(uri, v1, v2, HarbourProcessState);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, HarbourSystemInfo);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, HarbourSystemState);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, HarbourTransferMethodsModel);
    REGISTER_TYPE(uri, v1, v2, HarbourWakeupTimer);

    REGISTER_SINGLETON_TYPE(uri, v1, v2, FoilPicsSettings);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, FoilPics);
    REGISTER_TYPE(uri, v1, v2, FoilPicsBusyState);
    REGISTER_TYPE(uri, v1, v2, FoilPicsHints);
    REGISTER_TYPE(uri, v1, v2, FoilPicsModel);
    REGISTER_TYPE(uri, v1, v2, FoilPicsModelWatch);
    REGISTER_TYPE(uri, v1, v2, FoilPicsSelection);
    REGISTER_TYPE(uri, v1, v2, FoilPicsSelectionState);
}

int main(int argc, char *argv[])
{
    QGuiApplication* app = SailfishApp::application(argc, argv);
    app->setApplicationName(FOILPICS_APP_NAME);

    register_types(FOILPICS_QML_IMPORT, 1, 0);
    HarbourTransferMethodInfo2::registerTypes();

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

    // Initialize the view and the global properties
    view->setTitle(qtTrId("foilpics-app_name"));
    context->setContextProperty("MaximumHintCount", 1);
    //context->setContextProperty("AppSettings", new AppSettings(app));

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();

    int ret = app->exec();

    delete view;
    delete app;
    return ret;
}
