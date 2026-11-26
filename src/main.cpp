#include "editor_backend.h"
#include "gutter.h"
#include "tab_session.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QtGlobal>
#include <dlfcn.h>

static bool canUseWaylandVulkan() {
    if (!qEnvironmentVariableIsEmpty("NULLA_NO_VULKAN")) {
        return false;
    }
    const QString rhiEnv = qEnvironmentVariable("QSG_RHI_BACKEND").toLower();
    if (!rhiEnv.isEmpty() && rhiEnv != QLatin1String("opengl")) {
        return false;
    }
    if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        return false;
    }
    const QString qpa = qEnvironmentVariable("QT_QPA_PLATFORM").toLower();
    if (!qpa.isEmpty() && !qpa.contains(QLatin1String("wayland"))) {
        return false;
    }
    const bool hasIcd =
        !QDir(QStringLiteral("/usr/share/vulkan/icd.d")).entryList({QStringLiteral("*.json")}, QDir::Files).isEmpty() ||
        !QDir(QStringLiteral("/etc/vulkan/icd.d")).entryList({QStringLiteral("*.json")}, QDir::Files).isEmpty();
    if (!hasIcd) {
        return false;
    }
    void *vk = dlopen("libvulkan.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!vk) {
        return false;
    }
    dlclose(vk);
    return true;
}

int main(int argc, char *argv[]) {
    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP")) {
        qputenv("QSG_RENDER_LOOP", "basic");
    }
    if (canUseWaylandVulkan()) {
        qputenv("QSG_RHI_BACKEND", "vulkan");
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
    }
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Nulla"));
    app.setOrganizationName(QStringLiteral("Nulla"));
    if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                QStringLiteral("nulla.desktop")).isEmpty()) {
        app.setDesktopFileName(QStringLiteral("nulla"));
    }
    app.setWindowIcon(QIcon(QStringLiteral(":/assets/nulla.svg")));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    qmlRegisterType<EditorBackend>("Nulla", 1, 0, "EditorBackend");
    qmlRegisterType<LineNumberGutter>("Nulla", 1, 0, "LineNumberGutter");
    qmlRegisterUncreatableType<TabSession>("Nulla", 1, 0, "TabSession", QStringLiteral("Managed by EditorBackend"));
    qmlRegisterUncreatableType<TabModel>("Nulla", 1, 0, "TabModel", QStringLiteral("Managed by EditorBackend"));

    QStringList initialFiles;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (!args.at(i).isEmpty()) {
            initialFiles.append(args.at(i));
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilesArg"), initialFiles);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
