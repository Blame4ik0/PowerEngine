#include "Shader.h"
#include "Core/Logger.h"

namespace Engine
{
    bool Shader::Load(
        ID3D11Device* device,
        const std::wstring& filepath,
        const std::string& vsEntry,
        const std::string& psEntry)
    {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        // ---- Vertex Shader ----
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            filepath.c_str(),
            nullptr, nullptr,
            vsEntry.c_str(), "vs_5_0",
            flags, 0,
            m_vsBlob.GetAddressOf(),
            errorBlob.GetAddressOf()
        );

        if (FAILED(hr))
        {
            if (errorBlob)
                LOG_ERROR("VS compile error: {}",
                    (const char*)errorBlob->GetBufferPointer());
            else
                LOG_ERROR("VS compile failed, check shader path: {}",
                    std::string(filepath.begin(), filepath.end()));
            return false;
        }

        hr = device->CreateVertexShader(
            m_vsBlob->GetBufferPointer(),
            m_vsBlob->GetBufferSize(),
            nullptr,
            m_vertexShader.GetAddressOf());

        if (FAILED(hr)) { LOG_ERROR("CreateVertexShader failed."); return false; }

        // ---- Pixel Shader ----
        ComPtr<ID3DBlob> psBlob;
        hr = D3DCompileFromFile(
            filepath.c_str(),
            nullptr, nullptr,
            psEntry.c_str(), "ps_5_0",
            flags, 0,
            psBlob.GetAddressOf(),
            errorBlob.GetAddressOf()
        );

        if (FAILED(hr))
        {
            if (errorBlob)
                LOG_ERROR("PS compile error: {}",
                    (const char*)errorBlob->GetBufferPointer());
            else
                LOG_ERROR("PS compile failed — check shader path: {}",
                    std::string(filepath.begin(), filepath.end()));
            return false;
        }

        hr = device->CreatePixelShader(
            psBlob->GetBufferPointer(),
            psBlob->GetBufferSize(),
            nullptr,
            m_pixelShader.GetAddressOf());

        if (FAILED(hr)) { LOG_ERROR("CreatePixelShader failed."); return false; }

        m_loaded = true;
        LOG_INFO("Shader loaded: {}",
            std::string(filepath.begin(), filepath.end()));
        return true;
    }

    void Shader::Bind(ID3D11DeviceContext* ctx) const
    {
        ctx->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        ctx->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    }
}
