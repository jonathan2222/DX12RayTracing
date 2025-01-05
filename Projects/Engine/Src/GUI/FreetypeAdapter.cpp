#include "FreetypeAdapter.h"
#include "PreCompiled.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "GUI/IconsFontAwesome6.h"

std::shared_ptr<RS::FreetypeAdapter> RS::FreetypeAdapter::Get()
{
    static std::shared_ptr<FreetypeAdapter> s_Freetype = std::make_shared<FreetypeAdapter>();
    return s_Freetype;
}

void RS::FreetypeAdapter::Init()
{
    FT_Library pLibrary = nullptr;
    FT_Error error = FT_Init_FreeType(&pLibrary);
    RS_ASSERT(error == FT_Err_Ok, "Coult not initialize freetype!");

    // TODO: Move this to another function where we can specify which font to load!
    std::string faSolid900 = RS_FONT_PATH FONT_ICON_FILE_NAME_FAS;
    faSolid900 = Engine::GetInternalDataFilePath() + faSolid900;

    FT_Face pFace;
    error = FT_New_Face(pLibrary, faSolid900.c_str(), 0, &pFace);
    RS_ASSERT(error == FT_Err_Ok, "Coult not load font '{}' for freetype!", faSolid900.c_str());

    FT_Set_Pixel_Sizes(pFace, 0, 48); // Set size of this font.

    FT_GlyphSlot slot = pFace->glyph;

    m_Characters.clear();
    for (unsigned char c = 32; c < 255; c++)
    {
        error = FT_Load_Char(pFace, c, FT_LOAD_RENDER);
        if (error != FT_Err_Ok)
        {
            RS_LOG_WARNING("Failed to load Glyph '{}'", (char)c);
            continue;
        }

        error = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF);
        if (error != FT_Err_Ok)
        {
            RS_LOG_WARNING("Failed to render SDF glyph '{}'!", (char)c);
            continue;
        }

        // Width and height: slot->bitmap.width, slot->bitmap.rows
        // Data to draw: slot->bitmap->buffer

        Character character = {
            -1,
            slot->bitmap.width, slot->bitmap.rows,
            slot->bitmap_left, slot->bitmap_top,
            slot->advance.x
        };
        m_Characters.insert(std::pair<char, Character>((char)c, character));
    }

    FT_Done_Face(pFace);
    FT_Done_FreeType(pLibrary);
}

void RS::FreetypeAdapter::Release()
{
}
