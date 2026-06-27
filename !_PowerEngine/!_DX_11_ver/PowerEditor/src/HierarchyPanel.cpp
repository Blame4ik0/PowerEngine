#include "HierarchyPanel.h"
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

HierarchyPanel::HierarchyPanel(SceneDocument* doc, QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
{
    m_list = new QListWidget(this);

    m_addButton    = new QPushButton("Add Entity", this);
    m_removeButton = new QPushButton("Remove", this);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_removeButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_list);
    layout->addLayout(buttonRow);
    setLayout(layout);

    connect(m_list, &QListWidget::currentRowChanged,
            this, &HierarchyPanel::OnListSelectionChanged);
    connect(m_addButton, &QPushButton::clicked,
            this, &HierarchyPanel::OnAddClicked);
    connect(m_removeButton, &QPushButton::clicked,
            this, &HierarchyPanel::OnRemoveClicked);
    connect(m_doc, &SceneDocument::SceneChanged,
            this, &HierarchyPanel::OnSceneChanged);

    RefreshList();
}

void HierarchyPanel::OnSceneChanged()
{
    RefreshList();
}

void HierarchyPanel::RefreshList()
{
    m_list->blockSignals(true);
    m_list->clear();
    for (int i = 0; i < m_doc->EntityCount(); i++)
        m_list->addItem(m_doc->EntityName(i));
    m_list->blockSignals(false);
}

void HierarchyPanel::OnListSelectionChanged()
{
    emit EntitySelected(m_list->currentRow());
}

void HierarchyPanel::OnAddClicked()
{
    int idx = m_doc->AddEntity("New Entity");
    m_list->setCurrentRow(idx);
}

void HierarchyPanel::OnRemoveClicked()
{
    int row = m_list->currentRow();
    if (row >= 0)
        m_doc->RemoveEntity(row);
}
