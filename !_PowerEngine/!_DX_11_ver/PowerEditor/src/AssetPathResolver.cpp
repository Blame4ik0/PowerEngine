#include "AssetPathResolver.h"
#include <QDir>
#include <QFileInfo>

QString AssetPathResolver::s_assetsRoot;

void AssetPathResolver::SetAssetsRoot(const QString& absolutePath)
{
    s_assetsRoot = absolutePath;
}

QString AssetPathResolver::AssetsRoot()
{
    return s_assetsRoot;
}

QString AssetPathResolver::Resolve(const QString& enginePath)
{
    if (enginePath.isEmpty()) return enginePath;

    QFileInfo info(enginePath);
    if (info.isAbsolute() && info.exists())
        return enginePath; // already a valid absolute path, nothing to do

    if (s_assetsRoot.isEmpty())
        return enginePath; // no root configured — best effort, will likely fail to load

    // Strip everything up to and including the last "!_ASSETS/" segment
    // in the stored path, then re-base that remainder onto the
    // configured root. This works regardless of how many "../" segments
    // the engine-side path used, since we only care about what comes
    // after "!_ASSETS/".
    QString marker = "!_ASSETS/";
    int idx = enginePath.indexOf(marker);
    if (idx == -1)
    {
        // No marker found — fall back to naive relative resolution
        // against the assets root's parent, in case this is some other
        // kind of relative path we don't specifically recognize.
        return QDir(s_assetsRoot).filePath(enginePath);
    }

    QString afterMarker = enginePath.mid(idx + marker.length());
    QString resolved = QDir(s_assetsRoot).filePath(afterMarker);
    return QDir::cleanPath(resolved);
}
