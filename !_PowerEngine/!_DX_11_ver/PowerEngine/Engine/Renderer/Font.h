#pragma once
#include "Texture2D.h"
#include <string>
#include <array>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace Engine
{
    struct GlyphInfo
    {
        float u0, v0;
        float u1, v1;
        float offsetX;
        float offsetY;
        float advanceX;
        float width;
        float height;
    };

    class Font
    {
    public:
        static constexpr int FirstChar = 32;
        static constexpr int CharCount = 96;
        static constexpr int AtlasSize = 512;

        Font() = default;
        ~Font() = default;

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        bool Load(ID3D11Device* device,
            const std::string& filepath,
            float fontSize = 24.0f);

        const GlyphInfo& GetGlyph(char c) const;
        const Texture2D& GetAtlas()       const { return m_atlas; }
        float            GetFontSize()    const { return m_fontSize; }
        float            GetLineHeight()  const { return m_fontSize * 1.2f; }
        bool             IsLoaded()       const { return m_loaded; }

    private:
        Texture2D                        m_atlas;
        std::array<GlyphInfo, CharCount> m_glyphs{};
        float                            m_fontSize = 24.0f;
        bool                             m_loaded = false;
    };
}