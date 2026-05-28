#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "Font.h"
#include "Core/Logger.h"

#include <fstream>
#include <vector>

namespace Engine
{
    bool Font::Load(ID3D11Device* device,
        const std::string& filepath,
        float fontSize)
    {
        m_fontSize = fontSize;

        // ---- Read TTF file into memory ----
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            LOG_ERROR("Font: could not open '{}'.", filepath);
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> ttfBuffer(size);
        if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), size))
        {
            LOG_ERROR("Font: could not read '{}'.", filepath);
            return false;
        }

        // ---- Bake font to bitmap ----
        // stbtt_bakedchar holds raw glyph metrics from stb
        std::vector<stbtt_bakedchar> bakedChars(CharCount);
        std::vector<unsigned char>   bitmap(AtlasSize * AtlasSize);

        int result = stbtt_BakeFontBitmap(
            ttfBuffer.data(), 0,
            fontSize,
            bitmap.data(), AtlasSize, AtlasSize,
            FirstChar, CharCount,
            bakedChars.data());

        if (result == 0)
        {
            LOG_ERROR("Font: stbtt_BakeFontBitmap failed for '{}'."
                " Try a smaller font size or larger atlas.", filepath);
            return false;
        }

        // ---- Convert single-channel bitmap to RGBA ----
        // stb gives us greyscale — we expand to RGBA so it
        // works with our existing DXGI_FORMAT_R8G8B8A8_UNORM pipeline.
        // R=G=B=255 (white), A=greyscale value.
        // This way color tinting works correctly in the shader.
        std::vector<unsigned char> rgba(AtlasSize * AtlasSize * 4);
        for (int i = 0; i < AtlasSize * AtlasSize; i++)
        {
            rgba[i * 4 + 0] = 255;
            rgba[i * 4 + 1] = 255;
            rgba[i * 4 + 2] = 255;
            rgba[i * 4 + 3] = bitmap[i];
        }

        // ---- Upload atlas to GPU via Texture2D ----
        if (!m_atlas.LoadFromMemory(device, rgba.data(), AtlasSize, AtlasSize))
        {
            LOG_ERROR("Font: failed to upload atlas texture.");
            return false;
        }

        // ---- Convert stbtt_bakedchar to our GlyphInfo ----
        float atlasW = static_cast<float>(AtlasSize);
        float atlasH = static_cast<float>(AtlasSize);

        for (int i = 0; i < CharCount; i++)
        {
            const stbtt_bakedchar& bc = bakedChars[i];
            GlyphInfo& g = m_glyphs[i];

            g.u0 = bc.x0 / atlasW;
            g.v0 = bc.y0 / atlasH;
            g.u1 = bc.x1 / atlasW;
            g.v1 = bc.y1 / atlasH;
            g.offsetX = bc.xoff;
            g.offsetY = bc.yoff;
            g.advanceX = bc.xadvance;
            g.width = static_cast<float>(bc.x1 - bc.x0);
            g.height = static_cast<float>(bc.y1 - bc.y0);
        }

        m_loaded = true;
        LOG_INFO("Font loaded: '{}' at {}px.", filepath, fontSize);
        return true;
    }

    const GlyphInfo& Font::GetGlyph(char c) const
    {
        int idx = static_cast<int>(c) - FirstChar;
        if (idx < 0 || idx >= CharCount)
            idx = 0; // fallback to space
        return m_glyphs[idx];
    }
}