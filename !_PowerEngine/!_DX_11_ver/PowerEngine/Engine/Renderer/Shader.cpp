#include "Shader.h"
#include "Core/Logger.h"
#include <d3dcompiler.h>
#include <stdexcept>

namespace Engine
{
    bool Shader::Load(ID3D11Device* device,
        const std::wstring& filepath,
        const std::string& vsEntry,
        const std::string& psEntry)
    {
        UINT compileFlags = 0;
#ifdef _DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompileFromFile(
            filepath.c_str(),
            nullptr, nullptr,
            vsEntry.c_str(),
            "vs_5_0",
            compileFlags, 0,
            m_vsBlob.GetAddressOf(),
            errorBlob.GetAddressOf()
        );

        if (FAILED(hr))
        {
            if (errorBlob)
                LOG_ERROR("VS compile error: {}",
                    static_cast<const char*>(errorBlob->GetBufferPointer()));
            else
                LOG_ERROR("VS compile failed, no error info. Is the shader path correct?");
            return false;
        }

        hr = device->CreateVertexShader(
            m_vsBlob->GetBufferPointer(),
            m_vsBlob->GetBufferSize(),
            nullptr,
            m_vertexShader.GetAddressOf()
        );
        if (FAILED(hr)) { LOG_ERROR("CreateVertexShader failed."); return false; }

        ComPtr<ID3DBlob> psBlob;
        hr = D3DCompileFromFile(
            filepath.c_str(),
            nullptr, nullptr,
            psEntry.c_str(),
            "ps_5_0",
            compileFlags, 0,
            psBlob.GetAddressOf(),
            errorBlob.GetAddressOf()
        );

        if (FAILED(hr))
        {
            if (errorBlob)
                LOG_ERROR("PS compile error: {}",
                    static_cast<const char*>(errorBlob->GetBufferPointer()));
            else
                LOG_ERROR("PS compile failed, no error info. Is the shader path correct?");
            return false;
        }

        hr = device->CreatePixelShader(
            psBlob->GetBufferPointer(),
            psBlob->GetBufferSize(),
            nullptr,
            m_pixelShader.GetAddressOf()
        );
        if (FAILED(hr)) { LOG_ERROR("CreatePixelShader failed."); return false; }

        LOG_INFO("Shader loaded: {}", std::string(filepath.begin(), filepath.end()));
        return true;
    }

    void Shader::Bind(ID3D11DeviceContext* context) const
    {
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    }
}
