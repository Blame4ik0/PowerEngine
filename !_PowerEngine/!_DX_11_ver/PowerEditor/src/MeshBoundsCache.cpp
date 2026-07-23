#include "MeshBoundsCache.h"
#include "AssetPathResolver.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <QDebug>
#include <limits>
#include <cmath>

QHash<QString, MeshBoundsCache::Bounds> MeshBoundsCache::s_cache;

const MeshBoundsCache::Bounds& MeshBoundsCache::Get(const QString& filepath)
{
    auto it = s_cache.find(filepath);
    if (it != s_cache.end())
        return it.value();

    Bounds bounds;

    QString resolvedPath = AssetPathResolver::Resolve(filepath);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        resolvedPath.toStdString(),
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals);

    if (!scene || !scene->HasMeshes())
    {
        qWarning() << "MeshBoundsCache: failed to load" << filepath
                   << "(resolved to" << resolvedPath << ")"
                   << "-" << importer.GetErrorString();
        s_cache.insert(filepath, bounds);
        return s_cache[filepath];
    }

    float minX =  std::numeric_limits<float>::max();
    float minY =  std::numeric_limits<float>::max();
    float minZ =  std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    // IMPORTANT: iterate scene->mMeshes[] directly with NO node-tree
    // transform baking. This deliberately mirrors PowerEngine's own
    // Mesh::Load, which also reads mesh->mVertices raw and never walks
    // aiNode::mTransformation. Files where node transforms differ from
    // identity (common in glTF/GLB scene graphs — e.g. sword, angel,
    // knight) would otherwise be baked correctly here but rendered
    // without that baking in the engine, causing the editor and engine
    // to disagree on orientation/scale for exactly those files. Matching
    // the engine's (simpler, but authoritative) behavior keeps both
    // sides visually consistent, even though it means transforms on
    // non-trivial glTF scene graphs are technically not applied.
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;

            for (unsigned int vi = 0; vi < 3; vi++)
            {
                unsigned int idx = face.mIndices[vi];
                const aiVector3D& p = mesh->mVertices[idx];

                minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
                minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
                minZ = std::min(minZ, p.z); maxZ = std::max(maxZ, p.z);

                bounds.VertexData.push_back(p.x);
                bounds.VertexData.push_back(p.y);
                bounds.VertexData.push_back(p.z);

                if (mesh->HasNormals())
                {
                    const aiVector3D& n = mesh->mNormals[idx];
                    bounds.VertexData.push_back(n.x);
                    bounds.VertexData.push_back(n.y);
                    bounds.VertexData.push_back(n.z);
                }
                else
                {
                    bounds.VertexData.push_back(0.0f);
                    bounds.VertexData.push_back(1.0f);
                    bounds.VertexData.push_back(0.0f);
                }
            }
        }
    }

    if (minX <= maxX)
    {
        bounds.Min         = { minX, minY, minZ };
        bounds.Max         = { maxX, maxY, maxZ };
        bounds.Valid       = true;
        bounds.VertexCount = static_cast<int>(bounds.VertexData.size() / 6);
    }

    s_cache.insert(filepath, bounds);
    return s_cache[filepath];
}

void MeshBoundsCache::Clear()
{
    s_cache.clear();
}