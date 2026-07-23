#pragma once
#include <QMainWindow>
#include "SceneDocument.h"

class HierarchyPanel;
class InspectorPanel;
class RunSettingsDialog;
class ViewportWidget;
class ModelPreviewDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void OnNewScene();
    void OnOpenScene();
    void OnSaveScene();
    void OnSaveSceneAs();
    void OnRunClicked();
    void OnRunSettingsClicked();
    void OnPreviewModelClicked();
    void OnEntitySelected(int index);

private:
    void BuildMenusAndToolbar();
    void UpdateWindowTitle();
    QString GetEnginePath() const;
    void WriteEngineConfig(const QString& iniPath) const;

    SceneDocument*      m_doc;
    HierarchyPanel*     m_hierarchy;
    InspectorPanel*     m_inspector;
    RunSettingsDialog*  m_runSettings;
    ViewportWidget*     m_viewport;

    int     m_selectedEntityIndex = -1;
    QString m_currentScenePath; // empty until first Save/Open
};
