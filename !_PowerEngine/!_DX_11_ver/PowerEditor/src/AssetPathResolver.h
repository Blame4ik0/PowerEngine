#pragma once
#include <QString>

// Resolves the engine-style relative asset paths stored in scene files
// (e.g. "../../../../../../!_ASSETS/!_3D_models/sword/sword.glb") into
// absolute paths the editor can actually open.
//
// Why this exists: the engine finds !_ASSETS via a path relative to its
// own exe location deep inside out/build/.../bin/. The editor has no
// such relationship to that folder, so scene-stored relative paths are
// meaningless from the editor's working directory. Instead, the editor
// is told once (via Run Settings) where !_ASSETS actually lives on this
// machine, and this resolver strips the leading "../" segments and
// re-bases whatever remains onto that configured root.
class AssetPathResolver
{
public:
    // Call once at startup (and whenever Run Settings changes) with the
    // absolute path to the local !_ASSETS folder.
    static void SetAssetsRoot(const QString& absolutePath);
    static QString AssetsRoot();

    // Converts a possibly-relative, engine-style path into an absolute
    // path resolved against the configured Assets Root. If the input is
    // already absolute, or no root is configured, returns it unchanged.
    static QString Resolve(const QString& enginePath);

private:
    static QString s_assetsRoot;
};
