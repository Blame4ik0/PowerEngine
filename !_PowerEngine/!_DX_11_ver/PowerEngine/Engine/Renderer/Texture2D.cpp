#include "Texture2D.h"
#include "Core/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Engine
{
    bool Texture2D::Load(ID3D11Device* device, const std::string& filepath)
    {
        // Force 4 channels (RGBA) regardless of source format
        int channels = 0;
        unsigned char* data = stbi_load(
            filepath.c_str(),
            &m_width, &m_height,
            &channels, 4);

        if (!data)
        {
            LOG_ERROR("Texture2D: failed to load '{}': {}",
                filepath, stbi_failure_reason());
            return false;
        }

        // Describe the texture
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(m_width);
        desc.Height = static_cast<UINT>(m_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // Point DX11 at the pixel data
        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = data;
        initData.SysMemPitch = static_cast<UINT>(m_width * 4);

        HRESULT hr = device->CreateTexture2D(&desc, &initData, m_texture.GetAddressOf());
        stbi_image_free(data);  // Free CPU copy — GPU has it now

        if (FAILED(hr))
        {
            LOG_ERROR("Texture2D: CreateTexture2D failed for '{}'.", filepath);
            return false;
        }

        // Shader resource view — how the shader samples the texture
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(
            m_texture.Get(), &srvDesc, m_srv.GetAddressOf());

        if (FAILED(hr))
        {
            LOG_ERROR("Texture2D: CreateShaderResourceView failed for '{}'.", filepath);
            return false;
        }

        m_loaded = true;
        LOG_INFO("Texture2D loaded: '{}' ({}x{})", filepath, m_width, m_height);
        return true;
    }

    bool Texture2D::LoadWhite(ID3D11Device* device)
    {
        unsigned char pixel[4] = { 255, 255, 255, 255 };

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = pixel;
        initData.SysMemPitch = 4;

        HRESULT hr = device->CreateTexture2D(&desc, &initData,
            m_texture.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("LoadWhite: CreateTexture2D failed."); return false; }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(m_texture.Get(), &srvDesc,
            m_srv.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("LoadWhite: CreateSRV failed."); return false; }

        m_width = 1;
        m_height = 1;
        m_loaded = true;
        return true;
    }

    void Texture2D::Bind(ID3D11DeviceContext* ctx, unsigned int slot) const
    {
        ctx->PSSetShaderResources(slot, 1, m_srv.GetAddressOf());
    }
}