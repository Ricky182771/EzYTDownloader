#include "Application.h"
#include "MainWindow.h"
#include "DependencyChecker.h"
#include "DependencyDialog.h"

#include <QDebug>
#include <QTimer>

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setApplicationName(QStringLiteral("EzYTDownloader"));
    setApplicationVersion(QStringLiteral(APP_VERSION));
    setOrganizationName(QStringLiteral("EzYT"));
}

Application::~Application() = default;

int Application::run()
{
    // Kick off dependency check shortly after the event loop starts
    m_depChecker = new DependencyChecker(this);

    connect(m_depChecker, &DependencyChecker::allFound,
            this,         &Application::onDepsAllFound);
    connect(m_depChecker, &DependencyChecker::missingDeps,
            this,         &Application::onDepsMissing);

    QTimer::singleShot(0, m_depChecker, &DependencyChecker::checkAll);

    return exec();
}

void Application::onDepsAllFound()
{
    qDebug() << "[App] All dependencies found — launching main window.";

    m_mainWindow = new MainWindow;
    m_mainWindow->show();
}

void Application::onDepsMissing(const QStringList& missing)
{
    qWarning() << "[App] Missing dependencies:" << missing;

    auto* dlg = new DependencyDialog(missing);

    connect(dlg, &QDialog::rejected, this, [this]() {
        qDebug() << "[App] User dismissed dependency dialog, quitting.";
        quit();
    });

    connect(dlg, &QDialog::accepted, this, [this]() {
        // User clicked install — app should quit, they'll restart after install
        qDebug() << "[App] Install launched, quitting.";
        quit();
    });

    dlg->show();
}
