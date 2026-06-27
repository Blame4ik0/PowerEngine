#pragma once
#include <QWidget>
#include "SceneDocument.h"

class QDoubleSpinBox;
class QLineEdit;
class QLabel;

// Right-hand dock panel: shows editable fields for the currently
// selected entity. First pass covers Name + Transform only;
// Material/Light editing will be added once this round-trips correctly.
class InspectorPanel : public QWidget
{
    Q_OBJECT
public:
    explicit InspectorPanel(SceneDocument* doc, QWidget* parent = nullptr);

public slots:
    void SetSelectedEntity(int index);

private slots:
    void OnNameEdited();
    void OnTransformEdited();

private:
    void RefreshFields();
    QDoubleSpinBox* MakeSpinBox();

    SceneDocument* m_doc;
    int            m_currentIndex = -1;
    bool           m_updatingUI   = false; // guards against feedback loops

    QLineEdit* m_nameEdit;

    QDoubleSpinBox* m_posX, * m_posY, * m_posZ;
    QDoubleSpinBox* m_rotX, * m_rotY, * m_rotZ;
    QDoubleSpinBox* m_sclX, * m_sclY, * m_sclZ;

    QLabel* m_noSelectionLabel;
    QWidget* m_fieldsContainer;
};
