/*
 * companion/main.cpp
 *
 * Configurator companion app, based on the one for GTA:SA Android loader.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <vitasdk.h>
#include <vitaGL.h>
#include <imgui_vita.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#include <string>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>

#define CONFIG_FILE_PATH DATA_PATH"config.txt"

float leftStickDeadZone;
float rightStickDeadZone;
int fpsLock;
bool fakeAccel_enabled;

void resetSettings() {
    leftStickDeadZone = 0.11f;
    rightStickDeadZone = 0.11f;
    fpsLock = 0;
    fakeAccel_enabled = false;
}

inline int8_t is_dir(char* p) {
    DIR* filetest = opendir(p);
    if (filetest != NULL) {
        closedir(filetest);
        return 1;
    }
    return 0;
}

void loadConfig(void) {
    char buffer[30];
    int value;

    FILE *config = fopen(CONFIG_FILE_PATH, "r");

    if (config) {
        while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
            if (strcmp("leftStickDeadZone", buffer) == 0) leftStickDeadZone = ((float)value / 100.f);
            else if (strcmp("rightStickDeadZone", buffer) == 0) rightStickDeadZone = ((float)value / 100.f);
            else if (strcmp("fpsLock", buffer) == 0) fpsLock = value;
            else if (strcmp("fakeAccel_enabled", buffer) == 0) fakeAccel_enabled = (bool)value;
        }
        fclose(config);
    }
}

void saveConfig(void) {
    FILE *config = fopen(CONFIG_FILE_PATH, "w+");

    if (config) {
        fprintf(config, "%s %d\n", "leftStickDeadZone", (int)(leftStickDeadZone * 100.f));
        fprintf(config, "%s %d\n", "rightStickDeadZone", (int)(rightStickDeadZone * 100.f));
        fprintf(config, "%s %d\n", "fpsLock", (int)fpsLock);
        fprintf(config, "%s %d\n", "fakeAccel_enabled", (int)fakeAccel_enabled);
        fclose(config);
    }
}

char *options_descs[] = {
        "Deadzone for the left analog stick. Increase if you have stick drift issues.\nThe default value is: 0.11.", // leftStickDeadZone
        "Deadzone for the right analog stick. Increase if you have stick drift issues.\nThe default value is: 0.11.", // rightStickDeadZone
        "If you want to reduce FPS fluctuations and instead stay on a lower but constant level, you may try this.\nThe default value is: None.", // fpsLock
        "Use D-Pad up for switching weapons orientation and zero-g jump instead of accelerometer gestures.\nThe default value is: Disabled.", // fakeAccel_enabled
};

enum {
    OPT_DEADZONE_L,
    OPT_DEADZONE_R,
    OPT_FPSLOCK,
    OPT_FAKEACCEL
};

char *desc = nullptr;

void SetDescription(int i) {
    if (ImGui::IsItemHovered())
        desc = options_descs[i];
}

ImTextureID loadTex(const char* fname) {
    FILE* f = fopen(fname, "r");
    if (!f) return NULL;
    int image_width, image_height, depth;
    unsigned char* data = stbi_load_from_file(f, &image_width, &image_height, &depth, 0);
    fclose(f);

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return reinterpret_cast<ImTextureID>(image_texture);
}

bool FancyButton(const char* label, const ImVec2& pos, const ImVec2& size, ImU32 color_1, ImU32 color_2) {
    bool ret = false;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_2);
    ImGui::PushStyleColor(ImGuiCol_Border, color_1);
    ImGui::PushStyleColor(ImGuiCol_Text, color_1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::SetCursorPos(pos);
    if (ImGui::Button(label, size))
        ret = true;
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
    return ret;
}

#include "imgui_internal.h"

namespace ImGui {
    bool SelectableCentered(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg);
}

bool ImGui::SelectableCentered(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) // FIXME-OPT: Avoid if vertically clipped.
        PopClipRect();

    ImGuiID id = window->GetID(label);
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    ImVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrentLineTextBaseOffset;
    ImRect bb(pos, {pos.x + size.x, pos.y + size.y});
    ItemSize(bb);

    // Fill horizontal space.
    ImVec2 window_padding = window->WindowPadding;
    float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? GetWindowContentRegionMax().x : GetContentRegionMax().x;
    float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - window->DC.CursorPos.x);
    ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
    ImRect bb_with_spacing(pos, {pos.x + size_draw.x, pos.y + size_draw.y});
    if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
        bb_with_spacing.Max.x += window_padding.x;

    // Selectables are tightly packed together, we extend the box to cover spacing between selectable.
    float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
    float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
    float spacing_R = style.ItemSpacing.x - spacing_L;
    float spacing_D = style.ItemSpacing.y - spacing_U;
    bb_with_spacing.Min.x -= spacing_L;
    bb_with_spacing.Min.y -= spacing_U;
    bb_with_spacing.Max.x += spacing_R;
    bb_with_spacing.Max.y += spacing_D;
    if (!ItemAdd(bb_with_spacing, (flags & ImGuiSelectableFlags_Disabled) ? 0 : id))
    {
        if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
            PushColumnClipRect();
        return false;
    }

    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_Menu) button_flags |= ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoHoldingActiveID;
    if (flags & ImGuiSelectableFlags_MenuItem) button_flags |= ImGuiButtonFlags_PressedOnRelease;
    if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
    if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb_with_spacing, id, &hovered, &held, button_flags);
    if (flags & ImGuiSelectableFlags_Disabled)
        selected = false;

    // Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
    if (pressed || hovered)// && (g.IO.MouseDelta.x != 0.0f || g.IO.MouseDelta.y != 0.0f))
        if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerActiveMask)
        {
            g.NavDisableHighlight = true;
            //SetNavID(id, window->DC.NavLayerCurrent);
        }

    // Render
    if (hovered || selected)
    {
        const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        RenderFrame(bb_with_spacing.Min, bb_with_spacing.Max, col, false, 0.0f);
        RenderNavHighlight(bb_with_spacing, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
    } else {
        const ImU32 col = GetColorU32(ImGuiCol_TitleBgCollapsed);
        RenderFrame(bb_with_spacing.Min, bb_with_spacing.Max, col, false, 0.0f);
    }

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
    {
        PushColumnClipRect();
        bb_with_spacing.Max.x -= (GetContentRegionMax().x - max_x);
    }

    ImVec2 text_pos;
    text_pos.x = bb.Min.x + (((bb.Max.x - bb.Min.x) - (label_size.x)) / 2.f);
    text_pos.y = bb.Min.y + (((bb.Max.y - bb.Min.y) - (label_size.y)) / 2.f);

    if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
    RenderText(text_pos, label);
    if (flags & ImGuiSelectableFlags_Disabled) PopStyleColor();

    // Automatically close popups
    if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
        CloseCurrentPopup();
    return pressed;
}

int main(int argc, char *argv[]) {
    resetSettings();
    loadConfig();
    int exit_code = 0xDEAD;

    vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);

    ImTextureID bg = loadTex("app0:/data/configurator-bg.png");

    ImGui::CreateContext();
    ImGui_ImplVitaGL_Init();
    ImGui_ImplVitaGL_TouchUsage(false);
    ImGui_ImplVitaGL_GamepadUsage(true);
    ImGui::StyleColorsDark();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::GetIO().MouseDrawCursor = false;

    while (exit_code == 0xDEAD) {
        desc = nullptr;
        ImGui_ImplVitaGL_NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
        ImGui::SetNextWindowSize(ImVec2(960, 544), ImGuiSetCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});

        ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImDrawList* idl = ImGui::GetWindowDrawList();
        idl->AddImage(bg, {0,0}, {960,544});


        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

        ImGui::BeginGroup();

        ImGui::PushItemWidth(385);

        ImGui::SetCursorPos({73, 135});
        ImGui::SliderFloat("##leftStickDeadZone", &leftStickDeadZone, 0.01f, 0.5f, "%.2f");
        SetDescription(OPT_DEADZONE_L);

        ImGui::SetCursorPos({73, 195});
        ImGui::SliderFloat("##rightStickDeadZone", &rightStickDeadZone, 0.01f, 0.5f, "%.2f");
        SetDescription(OPT_DEADZONE_R);

        ImGui::PopItemWidth();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0,0});
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, IM_COL32(24, 193, 203, 66));
        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(13, 193, 204, 166));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(13, 193, 204, 166));

        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {3,10});
            ImGui::SetCursorPos({73, 255});
            if (ImGui::SelectableCentered("Enable##cheat_infiniteAmmo", fakeAccel_enabled, 0, {80,20}))
                fakeAccel_enabled = true;
            SetDescription(OPT_FAKEACCEL);
            ImGui::PopStyleVar(1);

            ImGui::SetCursorPos({157, 255});
            if (ImGui::SelectableCentered("Disable##cheat_infiniteAmmo", !fakeAccel_enabled, 0, {80,20}))
                fakeAccel_enabled = false;
            SetDescription(OPT_FAKEACCEL);
        }

        {
            ImGui::SetCursorPos({73, 313});
            if (ImGui::SelectableCentered("No Lock", fpsLock == 0, 0, {120,20}))
                fpsLock = 0;
            SetDescription(OPT_FPSLOCK);

            ImGui::SetCursorPos({210, 313});
            if (ImGui::SelectableCentered("45 FPS", fpsLock == 45, 0, {72,20}))
                fpsLock = 45;
            SetDescription(OPT_FPSLOCK);

            ImGui::SetCursorPos({298, 313});
            if (ImGui::SelectableCentered("30 FPS", fpsLock == 30, 0, {72,20}))
                fpsLock = 30;
            SetDescription(OPT_FPSLOCK);

            ImGui::SetCursorPos({386, 313});
            if (ImGui::SelectableCentered("25 FPS", fpsLock == 25, 0, {72,20}))
                fpsLock = 25;
            SetDescription(OPT_FPSLOCK);
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (desc) {
            ImGui::SetCursorPos({72, 390});
            ImGui::PushTextWrapPos(464);
            ImGui::Text(desc);
            ImGui::PopTextWrapPos();
        }

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        if (FancyButton("Save and Launch", {523, 394}, {187, 40}, IM_COL32(190,58,237,255), IM_COL32(190,58,237,40))) {
            saveConfig();
            exit_code = 1;
        }

        if (FancyButton("Reset Settings", {724, 394}, {187, 40}, IM_COL32(237,165,58,255), IM_COL32(237,165,58,40))) {
            resetSettings();
        }

        ImGui::EndGroup();

        ImGui::PopStyleVar();
        ImGui::End();
        ImGui::PopStyleVar(2);

        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_FALSE);
    }

    if (exit_code < 2) // Save
        saveConfig();

    if (exit_code % 2 == 1) // Launch
        sceAppMgrLoadExec("app0:/eboot.bin", NULL, NULL);

    return 0;
}
