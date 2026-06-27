#pragma once
#include <QMainWindow>
#include "SceneDocument.h"

class HierarchyPanel;
class InspectorPanel;
class RunSettingsDialog;

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

private:
    void BuildMenusAndToolbar();
    void UpdateWindowTitle();
    QString GetEnginePath() const;
    void WriteEngineConfig(const QString& iniPath) const;

    SceneDocument*      m_doc;
    HierarchyPanel*     m_hierarchy;
    InspectorPanel*     m_inspector;
    RunSettingsDialog*  m_runSettings;

    QString m_currentScenePath; // empty until first Save/Open
};
