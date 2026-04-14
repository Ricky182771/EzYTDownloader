#pragma once

#include <QApplication>

class MainWindow;
class DependencyChecker;

/// Application entry point — handles startup dependency check,
/// then shows either the main window or the dependency dialog.
class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application() override;

    int run();

private slots:
    void onDepsAllFound();
    void onDepsMissing(const QStringList& missing);

private:
    MainWindow*        m_mainWindow = nullptr;
    DependencyChecker* m_depChecker = nullptr;
};
