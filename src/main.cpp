#include "editor_backend.h"
#include "gutter.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
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

    QString initialFile;
    const QStringList args = app.arguments();
    if (args.size() > 1) {
        initialFile = args.at(1);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialFileArg"), initialFile);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
