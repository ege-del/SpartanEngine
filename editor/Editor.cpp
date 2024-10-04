/*
Copyright(c) 2016-2024 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ====================================
#include "Editor.h"
#include "Core/Engine.h"
#include "Core/Settings.h"
#include "ImGui/ImGuiExtension.h"
#include "ImGui/Implementation/ImGui_RHI.h"
#include "ImGui/Implementation/imgui_impl_sdl2.h"
#include "ImGui/Implementation/ImGui_Style.h"
#include "Widgets/AssetBrowser.h"
#include "Widgets/Console.h"
#include "Widgets/TitleBar.h"
#include "Widgets/ProgressDialog.h"
#include "Widgets/Properties.h"
#include "Widgets/Viewport.h"
#include "Widgets/WorldViewer.h"
#include "Widgets/ShaderEditor.h"
#include "Widgets/ResourceViewer.h"
#include "Widgets/Profiler.h"
#include "Widgets/RenderOptions.h"
//===============================================

//= NAMESPACES =====
using namespace std;
//==================

namespace
{
    float font_size             = 18.0f;
    float font_scale            = 1.0f;
    TitleBar* widget_menu_bar   = nullptr;
    Widget* widget_world        = nullptr;
    bool window_sponsor_visible = true;

    void process_event(Spartan::sp_variant data)
    {
        SDL_Event* event_sdl = static_cast<SDL_Event*>(get<void*>(data));
        ImGui_ImplSDL2_ProcessEvent(event_sdl);
    }

    void window_theme()
    {
        const float width  = 600.0f;
        const float height = 300.0f;

        // set position
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImVec2 window_pos   = ImVec2((display_size.x - width) * 0.25f, (display_size.y - height) * 0.25f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);

        if (ImGui::Begin("Theme"))
        {
            ImGui::Text("Test");

            ImGui::ColorEdit4("Background 1",&ImGui::Style::bg_1.x);
            ImGui::ColorEdit4("Background 2",&ImGui::Style::bg_2.x);

            ImGui::ColorEdit4("Highlight 1",&ImGui::Style::h_1.x);
            ImGui::ColorEdit4("Highlight 2",&ImGui::Style::h_2.x);

            ImGui::ColorEdit4("Accent 1",&ImGui::Style::color_accent_1.x);
            ImGui::ColorEdit4("Accent 2",&ImGui::Style::color_accent_2.x);

            if(ImGui::Button("Apply")){
                ImGui::Style::SetupImGuiStyle();
            }
        }
        ImGui::End();
    }

    void window_sponsor()
    {
        if (!window_sponsor_visible)
            return;

        const float width  = 600.0f;
        const float height = 300.0f;

        // set position
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImVec2 window_pos   = ImVec2((display_size.x - width) * 0.5f, (display_size.y - height) * 0.5f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        // set size
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
        // set focus
        ImGui::SetNextWindowFocus();

        if (ImGui::Begin("Support Spartan Engine", &window_sponsor_visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        {
            ImGui::TextWrapped(
                "I'm covering the costs for hosting the Spartan Engine's assets on my personal Dropbox, "
                "as well as running a dedicated website. If you enjoy the convenience of downloading the engine, "
                "running a single script, and getting all the necessary assets without hassle, "
                "please consider sponsoring me. Your support will help keep everything smooth and accessible!"
            );

            ImGui::Separator();

            if (ImGui::Button("Sponsor"))
            {
                Spartan::FileSystem::OpenUrl("https://github.com/sponsors/PanosK92");
            }
        }
        ImGui::End();
    }
}

Editor::Editor(const std::vector<std::string>& args)
{
    Spartan::Engine::Initialize(args);
    ImGui::CreateContext();

    // configure ImGui
    ImGuiIO& io                      = ImGui::GetIO();
    io.ConfigFlags                  |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags                  |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags                  |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags                  |= ImGuiConfigFlags_NoMouseCursorChange; // cursor control is given to ImGui, but dynamically, from the engine
    io.ConfigWindowsResizeFromEdges  = true;
    io.IniFilename                   = "editor.ini";

    // font_bold configuration
    ImFontConfig config; // config for bold font (mainly for use in headers)
    config.GlyphOffset.y = -2.0f;

    const string dir_fonts = Spartan::ResourceCache::GetResourceDirectory(Spartan::ResourceDirectory::Fonts) + "/";
    font_normal            = io.Fonts->AddFontFromFileTTF((dir_fonts + "OpenSans/OpenSans-Medium.ttf").c_str(), font_size * Spartan::Window::GetDpiScale());
    font_bold              = io.Fonts->AddFontFromFileTTF((dir_fonts + "OpenSans/OpenSans-Bold.ttf").c_str(), font_size * Spartan::Window::GetDpiScale(), &config);
    io.FontGlobalScale     = font_scale;

    // initialise imgui backends
    SP_ASSERT_MSG(ImGui_ImplSDL2_InitForVulkan(static_cast<SDL_Window*>(Spartan::Window::GetHandleSDL())), "Failed to initialize ImGui's SDL backend");
    ImGui::RHI::Initialize();

    // apply colors and style
    ImGui::Style::SetupImGuiStyle();
    // ImGui::Style::SetupImGuiStyleClassic();

    // initialization of some helper static classes
    IconLoader::Initialize();
    EditorHelper::Initialize(this);

    // create all imgui widgets
    m_widgets.emplace_back(make_shared<ProgressDialog>(this));
    m_widgets.emplace_back(make_shared<Console>(this));
    m_widgets.emplace_back(make_shared<Profiler>(this));
    m_widgets.emplace_back(make_shared<ResourceViewer>(this));
    m_widgets.emplace_back(make_shared<ShaderEditor>(this));
    m_widgets.emplace_back(make_shared<RenderOptions>(this));
    m_widgets.emplace_back(make_shared<TextureViewer>(this));
    m_widgets.emplace_back(make_shared<Viewport>(this));
    m_widgets.emplace_back(make_shared<AssetBrowser>(this));
    m_widgets.emplace_back(make_shared<Properties>(this));
    m_widgets.emplace_back(make_shared<WorldViewer>(this));
    widget_world = m_widgets.back().get();
    m_widgets.emplace_back(make_shared<TitleBar>(this));
    widget_menu_bar = static_cast<TitleBar*>(m_widgets.back().get());

    // allow imgui to get event's from the engine's event processing loop
    SP_SUBSCRIBE_TO_EVENT(Spartan::EventType::Sdl, SP_EVENT_HANDLER_VARIANT_STATIC(process_event));

    // register imgui as a third party library (will show up in the about window)
    Spartan::Settings::RegisterThirdPartyLib("ImGui", IMGUI_VERSION, "https://github.com/ocornut/imgui");

    // the sponsor window only shows up if the editor.ini file doesn't exist, which means that this is the first ever run
    window_sponsor_visible = !Spartan::FileSystem::Exists(io.IniFilename);
}

Editor::~Editor()
{
    if (ImGui::GetCurrentContext())
    {
        ImGui::RHI::shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    Spartan::Engine::Shutdown();
}

void Editor::Tick()
{
    // main loop
    while (!Spartan::Window::WantsToClose())
    {
        bool render_editor = Spartan::Engine::IsFlagSet(Spartan::EngineMode::EditorVisible);

        // logic
        {
            // imgui
            if (render_editor)
            {
                ImGui_ImplSDL2_NewFrame();
                ImGui::NewFrame();
            }

            // engine
            Spartan::Engine::Tick();

            window_sponsor();
            window_theme();

            // editor
            if (render_editor)
            {
                BeginWindow();

                for (shared_ptr<Widget>& widget : m_widgets)
                {
                    widget->Tick();
                }


                ImGui::End();
            }
        }

        // render
        if (render_editor)
        {
            ImGui::Render();

            if (Spartan::Renderer::CanUseCmdList())
            {
                // main window
                ImGui::RHI::render(ImGui::GetDrawData());
                Spartan::Renderer::Present();
            }

            // child windows
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }
    }
}

void Editor::BeginWindow()
{
    const auto window_flags =
        ImGuiWindowFlags_MenuBar               |
        ImGuiWindowFlags_NoDocking             |
        ImGuiWindowFlags_NoTitleBar            |
        ImGuiWindowFlags_NoCollapse            |
        ImGuiWindowFlags_NoResize              |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGuiStyle& style = ImGui::GetStyle();

    // set window position and size
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float padding_offset    = 2.0f * (style.FramePadding.y - TitleBar::GetPadding().y) - 1.0f;
    const float offset_y          = widget_menu_bar ? widget_menu_bar->GetHeight() + padding_offset : 0;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y - offset_y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - offset_y));
    ImGui::SetNextWindowViewport(viewport->ID);

    // set window style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // begin window
    std::string name = "##main_window";
    bool open = true;
    ImGui::Begin(name.c_str(), &open, window_flags);
    ImGui::PopStyleVar(3);

    // begin dock space
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        // dock space
        const auto window_id = ImGui::GetID(name.c_str());
        if (!ImGui::DockBuilderGetNode(window_id))
        {
            // reset current docking state
            ImGui::DockBuilderRemoveNode(window_id);
            ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
            ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);

            // dockBuilderSplitNode(ImGuiID node_id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_dir, ImGuiID* out_id_other);
            ImGuiID dock_main_id       = window_id;
            ImGuiID dock_right_id      = ImGui::DockBuilderSplitNode(dock_main_id,  ImGuiDir_Right, 0.17f, nullptr, &dock_main_id);
            ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down,  0.6f,  nullptr, &dock_right_id);
            ImGuiID dock_down_id       = ImGui::DockBuilderSplitNode(dock_main_id,  ImGuiDir_Down,  0.22f, nullptr, &dock_main_id);
            ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id,  ImGuiDir_Right, 0.5f,  nullptr, &dock_down_id);

            // dock windows
            ImGui::DockBuilderDockWindow("World",      dock_right_id);
            ImGui::DockBuilderDockWindow("Properties", dock_right_down_id);
            ImGui::DockBuilderDockWindow("Console",    dock_down_id);
            ImGui::DockBuilderDockWindow("Assets",     dock_down_right_id);
            ImGui::DockBuilderDockWindow("Viewport",   dock_main_id);

            ImGui::DockBuilderFinish(dock_main_id);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::PopStyleVar();
    }
}
