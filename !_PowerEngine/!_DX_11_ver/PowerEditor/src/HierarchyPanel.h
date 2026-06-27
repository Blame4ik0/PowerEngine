#pragma once
#include <QWidget>
#include "SceneDocument.h"

class QListWidget;
class QPushButton;

// Left-hand dock panel: lists every entity in the current scene.
// Selecting an item emits EntitySelected(index) so the InspectorPanel
// can display that entity's components.
class HierarchyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HierarchyPanel(SceneDocument* doc, QWidget* parent = nullptr);

signals:
    void EntitySelected(int index);

private slots:
    void OnSceneChanged();
    void OnListSelectionChanged();
    void OnAddClicked();
    void OnRemoveClicked();

private:
    void RefreshList();

    SceneDocument* m_doc;
    QListWidget*   m_list;
    QPushButton*   m_addButton;
    QPushButton*   m_removeButton;
};
