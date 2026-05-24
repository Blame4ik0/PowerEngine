#pragma once
#include <d3d11.h>
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

        bool Load(ID3D11Device* device,
            const std::wstring& filepath,
            const std::string& vsEntry,
            const std::string& psEntry);

        void Bind(ID3D11DeviceContext* context) const;

        ID3DBlob* GetVSBlob() const { return m_vsBlob.Get(); }

    private:
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader>  m_pixelShader;
        ComPtr<ID3DBlob>           m_vsBlob;
    };
}
