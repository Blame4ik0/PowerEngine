#include "SceneDocument.h"
#include <fstream>
#include <QDebug>

using json = nlohmann::json;

SceneDocument::SceneDocument(QObject* parent)
    : QObject(parent)
{
    NewScene();
}

void SceneDocument::NewScene()
{
    m_root = json::object();
    m_root["version"] = 1;
    m_root["entities"] = json::array();
    m_filepath.clear();
    m_dirty = false;
    emit SceneChanged();
}

bool SceneDocument::LoadFromFile(const QString& filepath)
{
    std::ifstream file(filepath.toStdString());
    if (!file.is_open())
    {
        qWarning() << "SceneDocument: could not open" << filepath;
        return false;
    }

    try
    {
        file >> m_root;
    }
    catch (const json::parse_error& e)
    {
        qWarning() << "SceneDocument: parse error in" << filepath << "-" << e.what();
        return false;
    }

    if (!m_root.contains("entities"))
        m_root["entities"] = json::array();

    m_filepath = filepath;
    m_dirty = false;
    emit SceneChanged();
    return true;
}

bool SceneDocument::SaveToFile(const QString& filepath) const
{
    std::ofstream file(filepath.toStdString());
    if (!file.is_open())
    {
        qWarning() << "SceneDocument: could not open" << filepath << "for writing";
        return false;
    }
    file << m_root.dump(2);
    return true;
}

int SceneDocument::EntityCount() const
{
    if (!m_root.contains("entities")) return 0;
    return static_cast<int>(m_root["entities"].size());
}

nlohmann::json& SceneDocument::EntityAt(int index)
{
    return m_root["entities"][index];
}

const nlohmann::json& SceneDocument::EntityAt(int index) const
{
    return m_root["entities"][index];
}

QString SceneDocument::EntityName(int index) const
{
    if (index < 0 || index >= EntityCount()) return QString();
    const auto& e = EntityAt(index);
    return QString::fromStdString(e.value("name", std::string("Entity")));
}

void SceneDocument::SetEntityName(int index, const QString& name)
{
    if (index < 0 || index >= EntityCount()) return;
    EntityAt(index)["name"] = name.toStdString();
    m_dirty = true;
    emit EntityModified(index);
}

void SceneDocument::RemoveEntity(int index)
{
    if (index < 0 || index >= EntityCount()) return;
    m_root["entities"].erase(m_root["entities"].begin() + index);
    m_dirty = true;
    emit SceneChanged();
}

int SceneDocument::AddEntity(const QString& name)
{
    json e;
    e["name"] = name.toStdString();
    e["transform"] = {
        { "position", { 0.0, 0.0, 0.0 } },
        { "rotation", { 0.0, 0.0, 0.0 } },
        { "scale",    { 1.0, 1.0, 1.0 } }
    };
    m_root["entities"].push_back(e);
    m_dirty = true;
    int newIndex = EntityCount() - 1;
    emit SceneChanged();
    return newIndex;
}
