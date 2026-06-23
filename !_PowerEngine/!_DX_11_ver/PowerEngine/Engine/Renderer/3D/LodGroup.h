#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include "Mesh.h"

namespace Engine
{
    // A single distance threshold + mesh pair.
    struct LodLevel
    {
        std::shared_ptr<Mesh> mesh;
        float                 maxDistance = 0.0f; // use this LOD while distance <= maxDistance
    };

    // Groups multiple pre-built meshes and picks one based on
    // distance from the camera. Levels must be added in order
    // from highest detail (smallest maxDistance) to lowest.
    class LodGroup
    {
    public:
        // maxDistance: switch away from this level once camera is farther than this
        void AddLevel(std::shared_ptr<Mesh> mesh, float maxDistance)
        {
            m_levels.push_back({ mesh, maxDistance });
        }

        // Returns the appropriate mesh for the given distance,
        // or the last (lowest detail) level if beyond all thresholds.
        Mesh* Select(float distanceToCamera) const
        {
            if (m_levels.empty()) return nullptr;
            for (auto& lvl : m_levels)
                if (distanceToCamera <= lvl.maxDistance)
                    return lvl.mesh.get();
            return m_levels.back().mesh.get(); // fall back to lowest detail
        }

        bool Empty() const { return m_levels.empty(); }

    private:
        std::vector<LodLevel> m_levels;
    };
}