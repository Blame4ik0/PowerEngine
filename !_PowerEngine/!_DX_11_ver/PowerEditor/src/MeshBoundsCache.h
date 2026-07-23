#pragma once
#include <QString>
#include <QVector3D>
#include <QHash>
#include <vector>

// Loads a mesh file via Assimp (geometry only) and caches:
//   - Local-space AABB (for gizmo sizing and accurate picking)
//   - Flattened vertex+normal list (for basic diffuse rendering in viewport)
// Results are keyed by filepath — each file is parsed at most once per session.
class MeshBoundsCache
{
public:
    struct Bounds
    {
        QVector3D Min   = { -0.5f, -0.5f, -0.5f };
        QVector3D Max   = {  0.5f,  0.5f,  0.5f };
        bool      Valid = false;

        // Interleaved vertex+normal data for GL rendering:
        // [x,y,z, nx,ny,nz,  x,y,z, nx,ny,nz, ...]  (6 floats per vertex)
        std::vector<float> VertexData;
        int                VertexCount = 0; // number of vertices (triangles * 3)
    };

    static const Bounds& Get(const QString& filepath);
    static void Clear();

private:
    static QHash<QString, Bounds> s_cache;
};
