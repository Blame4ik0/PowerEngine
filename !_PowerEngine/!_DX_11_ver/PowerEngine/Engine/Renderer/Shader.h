#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    class Shader
    {
    public:
        Shader() = default;
        ~Shader() = default;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        bool Load(
            ID3D11Device* device,
            const std::wstring& filepath,
            const std::string& vsEntry,
            const std::string& psEntry);

        void Bind(ID3D11DeviceContext* ctx) const;

        // Needed externally to create the input layout
        ID3DBlob* GetVSBlob() const { return m_vsBlob.Get(); }

        bool IsLoaded() const { return m_loaded; }

    private:
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader>  m_pixelShader;
        ComPtr<ID3DBlob>           m_vsBlob;
        bool                       m_loaded = false;
    };
}
