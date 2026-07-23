#include "RunSettingsDialog.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QSettings>
#include <QGroupBox>
#include <QLabel>

RunSettingsDialog::RunSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Run Settings");
    setMinimumWidth(480);

    auto* rootLayout = new QVBoxLayout(this);

    // ---- Build configuration group ----
    auto* buildGroup = new QGroupBox("Engine Build", this);
    auto* buildLayout = new QFormLayout(buildGroup);

    m_buildConfigCombo = new QComboBox(this);
    m_buildConfigCombo->addItems({ "Debug", "Release" });
    buildLayout->addRow("Active config", m_buildConfigCombo);

    auto makePathRow = [&](QLineEdit*& edit, void (RunSettingsDialog::*slot)())
    {
        edit = new QLineEdit(this);
        auto* browseBtn = new QPushButton("Browse...", this);
        auto* row = new QHBoxLayout();
        row->addWidget(edit);
        row->addWidget(browseBtn);
        auto* rowWidget = new QWidget();
        rowWidget->setLayout(row);
        connect(browseBtn, &QPushButton::clicked, this, slot);
        return rowWidget;
    };

    buildLayout->addRow("Debug exe",   makePathRow(m_debugPathEdit,   &RunSettingsDialog::OnBrowseDebug));
    buildLayout->addRow("Release exe", makePathRow(m_releasePathEdit, &RunSettingsDialog::OnBrowseRelease));

    rootLayout->addWidget(buildGroup);

    // ---- Assets Root group ----
    // The engine finds !_ASSETS via a relative path from its own exe
    // location; the editor has no such relationship, so it needs this
    // configured explicitly to resolve the same mesh/texture paths
    // stored in scene files (which stay relative for portability).
    auto* assetsGroup  = new QGroupBox("Assets", this);
    auto* assetsLayout = new QFormLayout(assetsGroup);
    assetsLayout->addRow("Assets Root (!_ASSETS folder)",
        makePathRow(m_assetsRootEdit, &RunSettingsDialog::OnBrowseAssetsRoot));
    rootLayout->addWidget(assetsGroup);

    // ---- Window / swapchain group ----
    auto* windowGroup = new QGroupBox("Window", this);
    auto* windowLayout = new QFormLayout(windowGroup);

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(320, 7680);
    m_widthSpin->setValue(1280);

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(240, 4320);
    m_heightSpin->setValue(720);

    auto* resRow = new QHBoxLayout();
    resRow->addWidget(m_widthSpin);
    resRow->addWidget(new QLabel("x", this));
    resRow->addWidget(m_heightSpin);
    auto* resWidget = new QWidget();
    resWidget->setLayout(resRow);
    windowLayout->addRow("Resolution", resWidget);

    m_vsyncCheck = new QCheckBox("Enabled", this);
    windowLayout->addRow("VSync", m_vsyncCheck);

    m_hzSpin = new QSpinBox(this);
    m_hzSpin->setRange(30, 1000);
    m_hzSpin->setValue(60);
    windowLayout->addRow("Target refresh rate (Hz)", m_hzSpin);

    m_showGridCheck = new QCheckBox("Visible on launch", this);
    m_showGridCheck->setChecked(true);
    windowLayout->addRow("Show Grid", m_showGridCheck);

    rootLayout->addWidget(windowGroup);

    // ---- OK / Cancel ----
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &RunSettingsDialog::OnAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    rootLayout->addWidget(buttons);

    LoadSettings();
}

void RunSettingsDialog::LoadSettings()
{
    QSettings s("PowerEngine", "PowerEditor");
    m_debugPathEdit->setText(s.value("Run/DebugExePath").toString());
    m_releasePathEdit->setText(s.value("Run/ReleaseExePath").toString());
    m_assetsRootEdit->setText(s.value("Run/AssetsRoot").toString());
    m_buildConfigCombo->setCurrentText(s.value("Run/Config", "Debug").toString());
    m_widthSpin->setValue(s.value("Run/Width", 1280).toInt());
    m_heightSpin->setValue(s.value("Run/Height", 720).toInt());
    m_vsyncCheck->setChecked(s.value("Run/VSync", false).toBool());
    m_hzSpin->setValue(s.value("Run/Hz", 60).toInt());
    m_showGridCheck->setChecked(s.value("Run/ShowGrid", true).toBool());
}

void RunSettingsDialog::SaveSettings()
{
    QSettings s("PowerEngine", "PowerEditor");
    s.setValue("Run/DebugExePath",   m_debugPathEdit->text());
    s.setValue("Run/ReleaseExePath", m_releasePathEdit->text());
    s.setValue("Run/AssetsRoot",     m_assetsRootEdit->text());
    s.setValue("Run/Config",         m_buildConfigCombo->currentText());
    s.setValue("Run/Width",  m_widthSpin->value());
    s.setValue("Run/Height", m_heightSpin->value());
    s.setValue("Run/VSync",  m_vsyncCheck->isChecked());
    s.setValue("Run/Hz",     m_hzSpin->value());
    s.setValue("Run/ShowGrid", m_showGridCheck->isChecked());
}

void RunSettingsDialog::OnBrowseDebug()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Select Debug PowerEngine.exe", QString(), "Executable (*.exe)");
    if (!path.isEmpty()) m_debugPathEdit->setText(path);
}

void RunSettingsDialog::OnBrowseRelease()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Select Release PowerEngine.exe", QString(), "Executable (*.exe)");
    if (!path.isEmpty()) m_releasePathEdit->setText(path);
}

void RunSettingsDialog::OnBrowseAssetsRoot()
{
    QString path = QFileDialog::getExistingDirectory(
        this, "Select !_ASSETS Folder", m_assetsRootEdit->text());
    if (!path.isEmpty()) m_assetsRootEdit->setText(path);
}

void RunSettingsDialog::OnAccept()
{
    SaveSettings();
    accept();
}

QString RunSettingsDialog::DebugExePath()   const { return m_debugPathEdit->text(); }
QString RunSettingsDialog::ReleaseExePath() const { return m_releasePathEdit->text(); }
bool    RunSettingsDialog::UseRelease()     const { return m_buildConfigCombo->currentText() == "Release"; }

QString RunSettingsDialog::ActiveExePath() const
{
    return UseRelease() ? ReleaseExePath() : DebugExePath();
}

int  RunSettingsDialog::Width()        const { return m_widthSpin->value();  }
int  RunSettingsDialog::Height()       const { return m_heightSpin->value(); }
bool RunSettingsDialog::VSync()        const { return m_vsyncCheck->isChecked(); }
int  RunSettingsDialog::RefreshRateHz() const { return m_hzSpin->value();    }
bool RunSettingsDialog::ShowGrid()      const { return m_showGridCheck->isChecked(); }

QString RunSettingsDialog::AssetsRoot() const { return m_assetsRootEdit->text(); }
