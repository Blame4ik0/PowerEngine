#include "Texture2D.h"
#include "Core/Logger.h"
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Engine
{
    // Full-chain mip count for a given width/height — same formula D3D uses.
    static UINT CalcMipLevels(int w, int h)
    {
        UINT levels = 1;
        while (w > 1 || h > 1) { w = std::max(1, w / 2); h = std::max(1, h / 2); levels++; }
        return levels;
    }

    bool Texture2D::Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::string& filepath)
    {
        int channels = 0;
        unsigned char* data = stbi_load(
            filepath.c_str(), &m_width, &m_height, &channels, 4);

        if (!data)
        {
            LOG_ERROR("Texture2D: failed to load '{}': {}",
                filepath, stbi_failure_reason());
            return false;
        }

        bool ok = LoadFromMemory(device, ctx, data, m_width, m_height);
        stbi_image_free(data);

        if (ok)
            LOG_INFO("Texture2D loaded: '{}' ({}x{}, {} mips)",
                filepath, m_width, m_height, m_mipLevels);
        return ok;
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
        m_mipLevels = 1;
        m_loaded = true;
        return true;
    }

    void Texture2D::Bind(ID3D11DeviceContext* ctx, unsigned int slot) const
    {
        ctx->PSSetShaderResources(slot, 1, m_srv.GetAddressOf());
    }

    bool Texture2D::LoadFromMemory(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const unsigned char* data,
        int width, int height)
    {
        m_width = width;
        m_height = height;

        // Auto mip generation requires DEFAULT usage + RENDER_TARGET bind
        // + GENERATE_MIPS misc flag. Only do this when ctx is provided —
        // otherwise fall back to a single-mip immutable texture.
        bool wantMips = (ctx != nullptr);
        m_mipLevels = wantMips ? CalcMipLevels(width, height) : 1;

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(width);
        desc.Height = static_cast<UINT>(height);
        desc.MipLevels = wantMips ? 0 : 1; // 0 = full chain, auto-generated
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = wantMips ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE
            | (wantMips ? D3D11_BIND_RENDER_TARGET : 0);
        desc.MiscFlags = wantMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

        // When using DEFAULT+GENERATE_MIPS, initial data must be uploaded
        // via UpdateSubresource after creation (can't pass it at create time
        // together with the auto-mip-gen misc flag in all driver paths).
        HRESULT hr;
        if (wantMips)
        {
            hr = device->CreateTexture2D(&desc, nullptr, m_texture.GetAddressOf());
            if (FAILED(hr))
            {
                LOG_ERROR("Texture2D: CreateTexture2D (mips) failed.");
                return false;
            }
            ctx->UpdateSubresource(m_texture.Get(), 0, nullptr,
                data, width * 4, 0);
        }
        else
        {
            D3D11_SUBRESOURCE_DATA initData{};
            initData.pSysMem = data;
            initData.SysMemPitch = static_cast<UINT>(width * 4);
            hr = device->CreateTexture2D(&desc, &initData, m_texture.GetAddressOf());
            if (FAILED(hr))
            {
                LOG_ERROR("Texture2D: CreateTexture2D failed.");
                return false;
            }
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = wantMips ? (UINT)-1 : 1; // -1 = all mips

        hr = device->CreateShaderResourceView(m_texture.Get(), &srvDesc,
            m_srv.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("Texture2D: CreateShaderResourceView failed.");
            return false;
        }

        if (wantMips)
            ctx->GenerateMips(m_srv.Get());

        m_loaded = true;
        return true;
    }

    bool Texture2D::LoadFromAssimp(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const aiTexture* tex)
    {
        if (!tex) return false;

        if (tex->mHeight == 0)
        {
            int w, h, channels;
            unsigned char* data = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(tex->pcData),
                static_cast<int>(tex->mWidth),
                &w, &h, &channels, 4);

            if (!data)
            {
                LOG_ERROR("Texture2D::LoadFromAssimp: stbi failed: {}",
                    stbi_failure_reason());
                return false;
            }

            bool ok = LoadFromMemory(device, ctx, data, w, h);
            stbi_image_free(data);
            return ok;
        }

        std::vector<unsigned char> rgba(tex->mWidth * tex->mHeight * 4);
        for (unsigned int i = 0; i < tex->mWidth * tex->mHeight; i++)
        {
            rgba[i * 4 + 0] = tex->pcData[i].r;
            rgba[i * 4 + 1] = tex->pcData[i].g;
            rgba[i * 4 + 2] = tex->pcData[i].b;
            rgba[i * 4 + 3] = tex->pcData[i].a;
        }

        return LoadFromMemory(device, ctx, rgba.data(),
            tex->mWidth, tex->mHeight);
    }
}