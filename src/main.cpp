#include "editor_backend.h"
#include "editor_view.h"
#include "tab_session.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QtEnvironmentVariables>
#include <QtGlobal>

#if defined(Q_OS_LINUX)
static bool isWaylandSession() {
  if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
    return false;
  }
  const QString qpa = qEnvironmentVariable("QT_QPA_PLATFORM").toLower();
  return qpa.isEmpty() || qpa.contains(QLatin1String("wayland"));
}

static void configureWaylandRenderer() {
  if (!isWaylandSession()) {
    return;
  }
  if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP")) {
    qputenv("QSG_RENDER_LOOP", "basic");
  }
  const QString rhiEnv = qEnvironmentVariable("QSG_RHI_BACKEND").toLower();
  if (rhiEnv == QLatin1String("vulkan")) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
  } else if (rhiEnv == QLatin1String("opengl") &&
             !qEnvironmentVariableIsEmpty("NULLA_USE_GPU")) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  } else if (rhiEnv.isEmpty() || rhiEnv == QLatin1String("opengl") ||
             rhiEnv == QLatin1String("software")) {
    qputenv("QSG_RHI_BACKEND", "software");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
  }
}
#endif

int main(int argc, char *argv[]) {
#if defined(Q_OS_LINUX)
  configureWaylandRenderer();
#endif
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("Nulla"));
  app.setOrganizationName(QStringLiteral("Nulla"));
#if defined(Q_OS_LINUX)
  if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                              QStringLiteral("nulla.desktop"))
           .isEmpty()) {
    app.setDesktopFileName(QStringLiteral("nulla"));
  }
#endif
  app.setWindowIcon(QIcon(QStringLiteral(":/assets/nulla.svg")));

  QQuickStyle::setStyle(QStringLiteral("Basic"));

  qmlRegisterType<EditorBackend>("Nulla", 1, 0, "EditorBackend");
  qmlRegisterType<NullaEditorView>("Nulla", 1, 0, "NullaEditorView");
  qmlRegisterUncreatableType<TabSession>(
      "Nulla", 1, 0, "TabSession", QStringLiteral("Managed by EditorBackend"));
  qmlRegisterUncreatableType<TabModel>(
      "Nulla", 1, 0, "TabModel", QStringLiteral("Managed by EditorBackend"));

  QStringList initialFiles;
  const QStringList args = app.arguments();
  for (int i = 1; i < args.size(); ++i) {
    if (!args.at(i).isEmpty()) {
      initialFiles.append(args.at(i));
    }
  }

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("initialFilesArg"),
                                           initialFiles);
  engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }

  return app.exec();
}
