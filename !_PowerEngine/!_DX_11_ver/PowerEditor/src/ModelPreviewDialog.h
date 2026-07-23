#pragma once
#include <QDialog>
#include <QString>
#include "SceneDocument.h"

class RunSettingsDialog;
class QComboBox;

// Generates a minimal temp scene (selected model + floor + directional sun)
// and launches PowerEngine.exe with it — no second renderer needed.
// Also exposes UV mode controls that write back to the entity in SceneDocument.
class ModelPreviewDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ModelPreviewDialog(SceneDocument* doc,
                                RunSettingsDialog* runSettings,
                                int entityIndex,
                                QWidget* parent = nullptr);

private slots:
    void OnPreviewClicked();
    void OnUVModeChanged(int index);

private:
    void GenerateTempScene(const QString& outPath) const;

    SceneDocument*     m_doc;
    RunSettingsDialog* m_runSettings;
    int                m_entityIndex;
    QComboBox*         m_uvModeCombo;
};
