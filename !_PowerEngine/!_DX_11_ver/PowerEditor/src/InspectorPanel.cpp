#include "InspectorPanel.h"
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>

InspectorPanel::InspectorPanel(SceneDocument* doc, QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
{
    auto* rootLayout = new QVBoxLayout(this);

    m_noSelectionLabel = new QLabel("No entity selected.", this);
    rootLayout->addWidget(m_noSelectionLabel);

    m_fieldsContainer = new QWidget(this);
    auto* fieldsLayout = new QVBoxLayout(m_fieldsContainer);

    // ---- Name ----
    auto* nameForm = new QFormLayout();
    m_nameEdit = new QLineEdit(m_fieldsContainer);
    nameForm->addRow("Name", m_nameEdit);
    fieldsLayout->addLayout(nameForm);

    // ---- Transform group ----
    auto* transformGroup = new QGroupBox("Transform", m_fieldsContainer);
    auto* transformLayout = new QFormLayout(transformGroup);

    auto addVec3Row = [&](const QString& label,
                          QDoubleSpinBox*& x, QDoubleSpinBox*& y, QDoubleSpinBox*& z)
    {
        x = MakeSpinBox();
        y = MakeSpinBox();
        z = MakeSpinBox();

        auto* row = new QHBoxLayout();
        row->addWidget(x);
        row->addWidget(y);
        row->addWidget(z);

        auto* rowWidget = new QWidget();
        rowWidget->setLayout(row);
        transformLayout->addRow(label, rowWidget);
    };

    addVec3Row("Position", m_posX, m_posY, m_posZ);
    addVec3Row("Rotation", m_rotX, m_rotY, m_rotZ);
    addVec3Row("Scale",    m_sclX, m_sclY, m_sclZ);

    fieldsLayout->addWidget(transformGroup);
    fieldsLayout->addStretch();

    rootLayout->addWidget(m_fieldsContainer);
    setLayout(rootLayout);

    connect(m_nameEdit, &QLineEdit::editingFinished,
            this, &InspectorPanel::OnNameEdited);

    for (auto* box : { m_posX, m_posY, m_posZ, m_rotX, m_rotY, m_rotZ, m_sclX, m_sclY, m_sclZ })
        connect(box, &QDoubleSpinBox::valueChanged,
                this, &InspectorPanel::OnTransformEdited);

    SetSelectedEntity(-1);
}

QDoubleSpinBox* InspectorPanel::MakeSpinBox()
{
    auto* box = new QDoubleSpinBox(this);
    box->setRange(-100000.0, 100000.0);
    box->setDecimals(3);
    box->setSingleStep(0.1);
    return box;
}

void InspectorPanel::SetSelectedEntity(int index)
{
    m_currentIndex = index;
    bool hasSelection = (index >= 0 && index < m_doc->EntityCount());

    m_noSelectionLabel->setVisible(!hasSelection);
    m_fieldsContainer->setVisible(hasSelection);

    if (hasSelection)
        RefreshFields();
}

void InspectorPanel::RefreshFields()
{
    if (m_currentIndex < 0) return;

    m_updatingUI = true;

    const auto& e = m_doc->EntityAt(m_currentIndex);

    m_nameEdit->setText(QString::fromStdString(
        e.value("name", std::string("Entity"))));

    if (e.contains("transform"))
    {
        const auto& t = e["transform"];
        const auto& pos = t.value("position", nlohmann::json::array({0,0,0}));
        const auto& rot = t.value("rotation", nlohmann::json::array({0,0,0}));
        const auto& scl = t.value("scale",    nlohmann::json::array({1,1,1}));

        m_posX->setValue(pos[0].get<double>());
        m_posY->setValue(pos[1].get<double>());
        m_posZ->setValue(pos[2].get<double>());

        m_rotX->setValue(rot[0].get<double>());
        m_rotY->setValue(rot[1].get<double>());
        m_rotZ->setValue(rot[2].get<double>());

        m_sclX->setValue(scl[0].get<double>());
        m_sclY->setValue(scl[1].get<double>());
        m_sclZ->setValue(scl[2].get<double>());
    }

    m_updatingUI = false;
}

void InspectorPanel::OnNameEdited()
{
    if (m_updatingUI || m_currentIndex < 0) return;
    m_doc->SetEntityName(m_currentIndex, m_nameEdit->text());
}

void InspectorPanel::OnTransformEdited()
{
    if (m_updatingUI || m_currentIndex < 0) return;

    auto& e = m_doc->EntityAt(m_currentIndex);
    e["transform"] = {
        { "position", { m_posX->value(), m_posY->value(), m_posZ->value() } },
        { "rotation", { m_rotX->value(), m_rotY->value(), m_rotZ->value() } },
        { "scale",    { m_sclX->value(), m_sclY->value(), m_sclZ->value() } }
    };

    emit m_doc->EntityModified(m_currentIndex);
}
