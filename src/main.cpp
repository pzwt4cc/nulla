#include "editor_backend.h"
#include "gutter.h"
#include "tab_session.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QtGlobal>

int main(int argc, char *argv[]) {
    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP")) {
        qputenv("QSG_RENDER_LOOP", "basic");
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
