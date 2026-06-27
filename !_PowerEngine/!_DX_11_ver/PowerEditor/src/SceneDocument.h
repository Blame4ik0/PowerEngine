#pragma once
#include <nlohmann/json.hpp>
#include <QString>
#include <QObject>
#include <vector>

// In-memory representation of a scene.json file, shared by the
// Hierarchy and Inspector panels. Emits signals when entities change
// so both panels stay in sync without re-parsing the file constantly.
//
// Deliberately thin: it wraps nlohmann::json directly rather than
// mapping to strongly-typed C++ structs, since the editor only ever
// needs to read/display/edit fields generically — the engine's
// SceneSerializer (C++ side) remains the single source of truth for
// what a "valid" scene looks like.
class SceneDocument : public QObject
{
    Q_OBJECT
public:
    explicit SceneDocument(QObject* parent = nullptr);

    bool LoadFromFile(const QString& filepath);
    bool SaveToFile(const QString& filepath) const;

    // Creates an empty scene (no entities) — used for "New Scene"
    void NewScene();

    int EntityCount() const;
    nlohmann::json& EntityAt(int index);
    const nlohmann::json& EntityAt(int index) const;

    QString EntityName(int index) const;
    void SetEntityName(int index, const QString& name);

    void RemoveEntity(int index);

    // Adds a bare entity with just a transform + name, returns its index.
    int AddEntity(const QString& name);

    const QString& CurrentFilepath() const { return m_filepath; }
    bool IsDirty() const { return m_dirty; }

signals:
    void SceneChanged();      // structural change: entity added/removed/loaded
    void EntityModified(int index); // a single entity's fields changed

private:
    nlohmann::json m_root;
    QString        m_filepath;
    bool           m_dirty = false;
};
