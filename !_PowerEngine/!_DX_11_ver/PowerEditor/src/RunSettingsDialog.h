#pragma once
#include <QDialog>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLineEdit;

// Editor-side dialog for engine launch settings: build config (paths to
// Debug/Release exe), window resolution, vsync, and target refresh rate.
// Settings persist via QSettings so they survive editor restarts.
//
// On "Run", MainWindow writes these values into engine_config.ini next
// to the chosen PowerEngine.exe — the engine reads that file at startup
// to configure its window/swapchain. This keeps "how to run" (this
// dialog) separate from "what to render" (scene.json).
class RunSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RunSettingsDialog(QWidget* parent = nullptr);

    // Loads/saves via QSettings (persisted across editor sessions)
    void LoadSettings();
    void SaveSettings();

    QString DebugExePath()   const;
    QString ReleaseExePath() const;
    bool    UseRelease()     const;
    QString ActiveExePath()  const; // resolves Debug/Release choice to a path

    int  Width()       const;
    int  Height()      const;
    bool VSync()        const;
    int  RefreshRateHz() const;

private slots:
    void OnBrowseDebug();
    void OnBrowseRelease();
    void OnAccept();

private:
    QComboBox* m_buildConfigCombo; // Debug / Release
    QLineEdit* m_debugPathEdit;
    QLineEdit* m_releasePathEdit;

    QSpinBox*  m_widthSpin;
    QSpinBox*  m_heightSpin;
    QCheckBox* m_vsyncCheck;
    QSpinBox*  m_hzSpin;
};
