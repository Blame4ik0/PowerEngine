#include "MainWindow.h"
#include "HierarchyPanel.h"
#include "InspectorPanel.h"
#include "RunSettingsDialog.h"
#include "ViewportWidget.h"
#include "ModelPreviewDialog.h"
#include "AssetPathResolver.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QLabel>
#include <QTextStream>
#include <QFile>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PowerEditor");
    resize(1280, 800);

    m_doc = new SceneDocument(this);
    m_runSettings = new RunSettingsDialog(this);
    AssetPathResolver::SetAssetsRoot(m_runSettings->AssetsRoot());

    // ---- Viewport (central widget) ----
    m_viewport = new ViewportWidget(m_doc, this);
    setCentralWidget(m_viewport);

    // ---- Hierarchy dock (left) ----
    m_hierarchy = new HierarchyPanel(m_doc, this);
    auto* hierarchyDock = new QDockWidget("Hierarchy", this);
    hierarchyDock->setWidget(m_hierarchy);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // ---- Inspector dock (right) ----
    m_inspector = new InspectorPanel(m_doc, this);
    auto* inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setWidget(m_inspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    connect(m_hierarchy, &HierarchyPanel::EntitySelected,
            m_inspector, &InspectorPanel::SetSelectedEntity);
    connect(m_hierarchy, &HierarchyPanel::EntitySelected,
            m_viewport, &ViewportWidget::SetSelectedEntity);
    connect(m_hierarchy, &HierarchyPanel::EntitySelected,
            this, &MainWindow::OnEntitySelected);
    connect(m_viewport, &ViewportWidget::EntityPicked,
            m_inspector, &InspectorPanel::SetSelectedEntity);
    connect(m_viewport, &ViewportWidget::EntityPicked,
            m_hierarchy, &HierarchyPanel::SelectRow);
    connect(m_viewport, &ViewportWidget::EntityPicked,
            this, &MainWindow::OnEntitySelected);

    BuildMenusAndToolbar();
    UpdateWindowTitle();
}

void MainWindow::BuildMenusAndToolbar()
{
    // ---- File menu ----
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New Scene",   this, &MainWindow::OnNewScene);
    fileMenu->addAction("Open Scene...", this, &MainWindow::OnOpenScene);
    fileMenu->addAction("Save",        this, &MainWindow::OnSaveScene);
    fileMenu->addAction("Save As...",  this, &MainWindow::OnSaveSceneAs);

    // ---- Toolbar with Run button ----
    auto* toolbar = addToolBar("Main");
    toolbar->addAction("New",  this, &MainWindow::OnNewScene);
    toolbar->addAction("Open", this, &MainWindow::OnOpenScene);
    toolbar->addAction("Save", this, &MainWindow::OnSaveScene);
    toolbar->addSeparator();
    toolbar->addAction("▶ Run", this, &MainWindow::OnRunClicked);
    toolbar->addAction("⚙ Run Settings", this, &MainWindow::OnRunSettingsClicked);
    toolbar->addSeparator();
    toolbar->addAction("🔍 Preview Model", this, &MainWindow::OnPreviewModelClicked);
}

void MainWindow::UpdateWindowTitle()
{
    QString name = m_currentScenePath.isEmpty()
        ? "Untitled" : m_currentScenePath;
    setWindowTitle("PowerEditor — " + name);
}

void MainWindow::OnNewScene()
{
    m_doc->NewScene();
    m_currentScenePath.clear();
    UpdateWindowTitle();
}

void MainWindow::OnOpenScene()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Scene", QString(), "PowerEngine Scene Files (*.pesf)");
    if (path.isEmpty()) return;

    if (!m_doc->LoadFromFile(path))
    {
        QMessageBox::warning(this, "Open Scene",
            "Failed to load scene file:\n" + path);
        return;
    }

    m_currentScenePath = path;
    UpdateWindowTitle();
}

void MainWindow::OnSaveScene()
{
    if (m_currentScenePath.isEmpty())
    {
        OnSaveSceneAs();
        return;
    }
    m_doc->SaveToFile(m_currentScenePath);
}

void MainWindow::OnSaveSceneAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Scene As", QString(), "PowerEngine Scene Files (*.pesf)");
    if (path.isEmpty()) return;

    if (!m_doc->SaveToFile(path))
    {
        QMessageBox::warning(this, "Save Scene",
            "Failed to save scene file:\n" + path);
        return;
    }

    m_currentScenePath = path;
    UpdateWindowTitle();
}

void MainWindow::OnRunSettingsClicked()
{
    m_runSettings->exec(); // modal — saves to QSettings on OK
    AssetPathResolver::SetAssetsRoot(m_runSettings->AssetsRoot());
    m_viewport->update(); // re-render with newly resolvable mesh paths
}

QString MainWindow::GetEnginePath() const
{
    return m_runSettings->ActiveExePath();
}

void MainWindow::WriteEngineConfig(const QString& iniPath) const
{
    // Plain key=value ini — simple to parse on the C++ engine side
    // without pulling in a full ini-parsing library.
    QFile file(iniPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "MainWindow: could not write engine_config.ini at" << iniPath;
        return;
    }

    QTextStream out(&file);
    out << "; Auto-generated by PowerEditor — do not edit while engine is running\n";
    out << "width="   << m_runSettings->Width()        << "\n";
    out << "height="  << m_runSettings->Height()       << "\n";
    out << "vsync="   << (m_runSettings->VSync() ? 1 : 0) << "\n";
    out << "hz="      << m_runSettings->RefreshRateHz()  << "\n";
    out << "show_grid=" << (m_runSettings->ShowGrid() ? 1 : 0) << "\n";
}

void MainWindow::OnRunClicked()
{
    // The engine needs a real file on disk to load from, so make sure
    // the current scene is saved first (Save As if it's never been saved).
    if (m_currentScenePath.isEmpty())
    {
        OnSaveSceneAs();
        if (m_currentScenePath.isEmpty())
            return; // user cancelled the save dialog
    }
    else
    {
        m_doc->SaveToFile(m_currentScenePath);
    }

    QString exePath = GetEnginePath();
    QFileInfo exeInfo(exePath);

    if (!exeInfo.exists())
    {
        QMessageBox::warning(this, "Run",
            "PowerEngine.exe not found at:\n" + exePath +
            "\n\nCheck Run Settings.");
        return;
    }

    // Write engine_config.ini next to the exe — this is how window/swapchain
    // settings (vsync, hz, resolution) reach the engine, separate from the
    // scene file which only describes entities.
    WriteEngineConfig(exeInfo.absolutePath() + "/engine_config.ini");

    QProcess* process = new QProcess(this);
    // CRITICAL: working directory must be the exe's own folder, since the
    // engine loads shaders/assets via paths relative to its own location
    // (e.g. "Shaders/Polygon.hlsl"). QProcess::startDetached does NOT do
    // this automatically — it inherits the editor's working directory,
    // which is why shader loading failed before this fix.
    process->setWorkingDirectory(exeInfo.absolutePath());
    process->setProgram(exePath);
    process->setArguments({ "--scene=" + m_currentScenePath });

    bool started = process->startDetached();
    if (!started)
    {
        QMessageBox::warning(this, "Run",
            "Failed to launch PowerEngine.exe.\nPath:\n" + exePath);
    }
    process->deleteLater();
}

void MainWindow::OnEntitySelected(int index)
{
    m_selectedEntityIndex = index;
}

void MainWindow::OnPreviewModelClicked()
{
    if (m_selectedEntityIndex < 0 ||
        m_selectedEntityIndex >= m_doc->EntityCount())
    {
        QMessageBox::information(this, "Preview Model",
            "Select an entity with a mesh first.");
        return;
    }

    const auto& e = m_doc->EntityAt(m_selectedEntityIndex);
    if (!e.contains("meshSource") ||
        e["meshSource"].value("type", "") != "file")
    {
        QMessageBox::information(this, "Preview Model",
            "Selected entity has no file-based mesh.\n"
            "Primitives (plane/cube/sphere) don't need a preview.");
        return;
    }

    ModelPreviewDialog dlg(m_doc, m_runSettings,
                            m_selectedEntityIndex, this);
    dlg.exec();
}
