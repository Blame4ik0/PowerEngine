#pragma once
#include "Renderer/RenderContext.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture2D.h"
#include "Renderer/2D/Renderer2D.h"
#include "Mesh.h"
#include "LodGroup.h"
#include "Camera3D.h"
#include "Light.h"
#include "ShadowMap.h"
#include "PointShadowMap.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Engine
{
    enum class RenderPass { Shadow, Main };

    // 6 frustum planes extracted from view-projection matrix
    struct Frustum
    {
        DirectX::XMFLOAT4 planes[6]; // xyz=normal, w=distance
        void Extract(const DirectX::XMMATRIX& viewProj);
        bool Intersects(const AABB& aabb,
            const DirectX::XMMATRIX& world) const;
    };

    class Renderer3D
    {
    public:
        static constexpr int MaxPointLights = 8;
        static constexpr int MaxInstanceCount = 1024;

        Renderer3D() = default;
        ~Renderer3D() = default;

        Renderer3D(const Renderer3D&) = delete;
        Renderer3D& operator=(const Renderer3D&) = delete;

        bool Init(RenderContext* context, const std::wstring& shaderPath);
        void Shutdown();

        void BeginScene(const Camera3D& camera);

        void SetDirectionalLight(const DirectionalLight& light);
        void AddPointLight(const PointLight& light);
        void ClearPointLights();

        void BeginShadowPass();
        void EndShadowPass();

        void BeginPointShadowPass();
        void EndPointShadowPass();
        void RenderPointShadowFace(int lightIndex, int face);

        void SetPointShadowQuality(PointShadowQuality quality);
        void SetShadowQuality(ShadowQuality quality);

        PointShadowQuality GetPointShadowQuality() const
        {
            return m_pointShadowMap.GetQuality();
        }
        ShadowQuality GetShadowQuality() const
        {
            return m_shadowMap.GetQuality();
        }

        void EnablePointShadows(bool e) { m_pointShadowsEnabled = e; }
        void EnableShadows(bool e) { m_shadowsEnabled = e; }
        void EnableFrustumCull(bool e) { m_frustumCullEnabled = e; }

        bool ShadowsEnabled()      const { return m_shadowsEnabled; }
        bool PointShadowsEnabled() const { return m_pointShadowsEnabled; }
        bool FrustumCullEnabled()  const { return m_frustumCullEnabled; }

        // Returns number of draw calls culled last frame
        int GetCulledCount() const { return m_culledCount; }

        // ---- Single-instance draw ----
        void DrawMesh(const Mesh& mesh,
            const DirectX::XMMATRIX& worldMatrix,
            const Material& material = Material{});

        void DrawMesh(const Mesh& mesh,
            float x, float y, float z,
            float rotX = 0, float rotY = 0, float rotZ = 0,
            float scaleX = 1, float scaleY = 1, float scaleZ = 1,
            const Material& material = Material{});

        void DrawMeshAuto(const Mesh& mesh,
            const DirectX::XMMATRIX& worldMatrix);

        // Picks the right LOD level based on distance to camera, then draws it
        void DrawLodGroup(const class LodGroup& group,
            const DirectX::XMMATRIX& worldMatrix,
            const Material& material = Material{});

        // ---- Instanced draw ----
        // Renders the same mesh+material N times with different world matrices
        // in a single DrawIndexedInstanced call. Up to MaxInstanceCount per call.
        void DrawMeshInstanced(const Mesh& mesh,
            const Material& material,
            const std::vector<DirectX::XMMATRIX>& transforms);

        void DebugDrawShadowMap(Renderer2D& renderer2D);
        void OnResize(float aspectRatio);

    private:
        void UpdateLightBuffer();
        void BindMainPassState(ID3D11DeviceContext* ctx);
        Texture2D* GetOrLoadTexture(const std::string& path);
        Texture2D* ResolveTexture(const std::shared_ptr<Texture2D>& direct,
            const std::string& path);

        RenderContext* m_context = nullptr;
        Shader         m_shader;

        // Constant buffers
        ComPtr<ID3D11Buffer> m_cbPerObject;
        ComPtr<ID3D11Buffer> m_cbPerFrame;
        ComPtr<ID3D11Buffer> m_cbLight;
        ComPtr<ID3D11Buffer> m_cbMaterial;
        ComPtr<ID3D11Buffer> m_cbShadow;
        ComPtr<ID3D11Buffer> m_cbShadowPass;
        ComPtr<ID3D11Buffer> m_cbPointShadow;
        ComPtr<ID3D11Buffer> m_cbPointShadowPass;

        // Pipeline state
        ComPtr<ID3D11InputLayout>       m_inputLayout;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;
        ComPtr<ID3D11SamplerState>      m_sampler;

        // Instancing
        ComPtr<ID3D11Buffer>            m_instanceBuffer;

        // Camera
        DirectX::XMMATRIX  m_view;
        DirectX::XMMATRIX  m_projection;
        DirectX::XMFLOAT3  m_cameraPosition;

        // Frustum culling
        Frustum            m_frustum;
        bool               m_frustumCullEnabled = true;
        int                m_culledCount = 0;

        // Lights
        DirectionalLight         m_dirLight;
        std::vector<PointLight>  m_pointLights;

        ShadowMap      m_shadowMap;
        PointShadowMap m_pointShadowMap;

        RenderPass m_currentPass = RenderPass::Main;
        bool       m_shadowsEnabled = true;
        bool       m_pointShadowsEnabled = true;

        bool m_inPointShadowPass = false;
        int  m_activeShadowLight = 0;
        int  m_activeShadowFace = 0;

        std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textureCache;
        std::shared_ptr<Texture2D> m_whiteTexture;
    };
}