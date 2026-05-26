#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    class Texture2D
    {
    public:
        Texture2D() = default;
        ~Texture2D() = default;

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        bool Load(ID3D11Device* device, const std::string& filepath);
        bool LoadWhite(ID3D11Device* device);
        void Bind(ID3D11DeviceContext* ctx, unsigned int slot = 0) const;

        bool IsLoaded()  const { return m_loaded; }
        int  GetWidth()  const { return m_width; }
        int  GetHeight() const { return m_height; }

        ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

    private:
        ComPtr<ID3D11Texture2D>          m_texture;
        ComPtr<ID3D11ShaderResourceView> m_srv;

        int  m_width = 0;
        int  m_height = 0;
        bool m_loaded = false;
    };
}
