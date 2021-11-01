#pragma once
#include "pch.h"
#include "graphics/render.h"

#include <windows.h>
#include <shellapi.h>
#include "utils/utils.h"
#include "extra/IconFontCppHeaders/IconsFontAwesome4.h"
#include "extra/IconFontCppHeaders/IconsFontAwesome5.h"
#include "extra/imgui-docking/imgui_internal.h"

namespace ImGui {
  namespace {
    unsigned int idx = 0u;
    int node_clicked = -1;
    int node_context = -1;
    static int selection_mask = (1 << 2);
    fs::path modal_path;


    const char* GetIconForExt(std::string ext) {
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

      if (ext == ".ini") return ICON_FA_COGS;

      if (ext == ".dds") return ICON_FA_FILE_IMAGE;
      if (ext == ".tga") return ICON_FA_FILE_IMAGE;
      if (ext == ".png") return ICON_FA_FILE_IMAGE;
      if (ext == ".jpg") return ICON_FA_FILE_IMAGE;
      if (ext == ".ico") return ICON_FA_FILE_IMAGE;

      if (ext == ".json")return ICON_FA_FILE_CODE;
      if (ext == ".js")  return ICON_FA_FILE_CODE;
      if (ext == ".cpp") return ICON_FA_FILE_CODE;
      if (ext == ".c")   return ICON_FA_FILE_CODE;
      if (ext == ".hpp") return ICON_FA_FILE_CODE;
      if (ext == ".h")   return ICON_FA_FILE_CODE;
      if (ext == ".lua") return ICON_FA_FILE_CODE;

      if (ext == ".ttf") return ICON_FA_FONT;
      if (ext == ".otf") return ICON_FA_FONT;

      if (ext == ".hlsl") return ICON_FA_ADJUST;
      if (ext == ".inc")  return ICON_FA_ADJUST;

      if (ext == ".mp3")  return ICON_FA_MUSIC;
      if (ext == ".wav")  return ICON_FA_MUSIC;

      return ICON_FA_QUESTION_CIRCLE;
    }

    void CMD_OpenItem(fs::path path)
    {
      const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
      HINSTANCE  result = ShellExecute(0, "open", path.u8string().c_str(), 0, 0, SW_SHOWNORMAL);
      if (result == (HINSTANCE)0x000000000000001f)
       result = ShellExecute(0, "edit", path.u8string().c_str(), 0, 0, SW_SHOWNORMAL);

      if (result == (HINSTANCE)0x000000000000002a)
        dbg("[Edit] %s", path.filename().u8string().c_str());
    }

    void CMD_Rename(fs::path path, std::string name)
    {
      const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
      HINSTANCE result = ShellExecute(0, 0, "cmd.exe", std::string("/c rename " + path.u8string() + " " + (path.parent_path() / name).u8string()).c_str(), 0, SW_SHOWNORMAL);
    }

    void CMD_NewFolder(fs::path path, std::string folder_name)
    {
      const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
      fs::path new_folder_path = path / folder_name;
      HINSTANCE result = ShellExecute(0, 0, "cmd.exe", std::string("/c mkdir " + new_folder_path.u8string()).c_str(), 0, SW_SHOWNORMAL);
    }

    void CMD_RemoveFolder(fs::path path) {
      const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
      HINSTANCE result = ShellExecute(0, 0, "cmd.exe", std::string("/c rmdir meshes /s/q " + path.u8string()).c_str(), 0, SW_SHOWNORMAL);
    }

    void CMD_ExploreFolder(fs::path path) {
      const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
      HINSTANCE result = ShellExecute(0, "explore", path.u8string().c_str(), 0, 0, SW_SHOWNORMAL);
    }

    void MODAL_NewFolder(fs::path path) {
      if (ImGui::BeginPopupModal("New Folder"))
      {
        ImGui::Text((path.u8string() + "\n\n").c_str());
        ImGui::Separator();

        static char buf1[64] = "new_folder"; ImGui::InputText("folder name", buf1, 64);

        if (ImGui::Button("OK", ImVec2(120, 0))
          || ImGui::IsKeyPressed(ImGuiKey_Enter)
          || ImGui::IsKeyPressed(ImGuiKey_Space)
          ) {
          CMD_NewFolder(path, buf1);
          ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
    }

    void MODAL_DeleteFolder(fs::path path) {
      if (ImGui::BeginPopupModal("Confirm Delete"))
      {
        ImGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!\n\n");
        ImGui::Separator();

        static bool dont_ask_me_next_time = false;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
        ImGui::PopStyleVar();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
          CMD_RemoveFolder(path);
          ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
      }
    }

    void ItemMenu(fs::path path)
    {
      static float value = 0.f;
      if (ImGui::BeginPopupContextItem("item context menu"))
      {
        //ImGui::Text("%s",path.u8string().c_str());
        if (ImGui::Selectable("Set to zero")) value = 0.0f;
        if (ImGui::Selectable("Set to PI")) value = 3.1415f;
        if (ImGui::Button("Delete File"))         ImGui::OpenPopup("Confirm Delete");
        if (ImGui::Selectable("Open in Explorer"))  CMD_ExploreFolder(path);
        MODAL_DeleteFolder(path);
        ImGui::EndPopup();
      }
    }

    void FolderMenu(fs::path path)
    {
      if (ImGui::BeginPopupContextItem("folder context menu"))
      {
        if (ImGui::Button("Create Folder"))         ImGui::OpenPopup("New Folder");
        if (ImGui::Button("Remove Folder"))         ImGui::OpenPopup("Confirm Delete");
        if (ImGui::Selectable("Open in Explorer"))  CMD_ExploreFolder(path);
        MODAL_NewFolder(path);
        MODAL_DeleteFolder(path);
        ImGui::EndPopup();
      }
    }

    void callback(const fs::path& path, unsigned int depth, unsigned int& idx)
    {
      for (auto&& p : fs::directory_iterator(path))
      {
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ((selection_mask & (1 << idx)) ? ImGuiTreeNodeFlags_Selected : 0);
        if (fs::is_directory(p.path()))
        {
          using namespace std::string_literals;

          std::string s = ICON_FA_FOLDER + " "s + p.path().filename().string().c_str();

          if (TreeNodeBehaviorIsOpen(GetID((void*)(intptr_t)idx), node_flags))
            s = ICON_FA_FOLDER_OPEN + " "s + p.path().filename().string().c_str();


          if (ImGui::TreeNodeEx((void*)(intptr_t)idx, node_flags, "%s", s.c_str()))
          {
            if (ImGui::IsItemClicked() || ImGui::IsItemFocused())
              node_clicked = idx;

            if (ImGui::IsItemClicked(1) || node_context == idx) {
              node_context = idx;
              FolderMenu(fs::absolute(p));
            }

            callback(p, depth + 1, ++idx);
            ImGui::TreePop();
          }
          else if (ImGui::IsItemClicked(1) || node_context == idx) {
            node_context = idx;
            FolderMenu(fs::absolute(p));
          }


        }
        else if (p.path().filename().string().rfind(".", 0) != 0)
        {
          if (depth > 0) ImGui::Indent();

          node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
          const auto icon = GetIconForExt(p.path().extension().string());
          ImGui::TreeNodeEx((void*)(intptr_t)idx, node_flags, "%s %s", icon, p.path().filename().u8string().c_str());

          if (ImGui::IsItemClicked() || ImGui::IsItemFocused())         node_clicked = idx;
          if (ImGui::IsMouseDoubleClicked(0) && node_clicked == idx)   CMD_OpenItem(fs::absolute(p));
          if (ImGui::IsItemClicked(1) || node_context == idx) {
            node_context = idx;
            ItemMenu(fs::absolute(p));
          }


          if (depth > 0) ImGui::Unindent();
        }
        ++idx;
      }
      depth -= 1;
    }
  }
  static void TreeView(fs::path path) {

    

    //Reset vars
    idx = 0u;
    node_clicked = -1;
    selection_mask = (1 << 2);

    if (ImGui::BeginPopupContextItem("root context menu"))
    {
      //if (ImGui::Button("Create Folder"))         ImGui::OpenPopup("New Folder");
      //if (ImGui::Button("Remove Folder"))         ImGui::OpenPopup("Confirm Delete");
      //if (ImGui::Selectable("Open in Explorer"))  CMD_ExploreFolder(fs::absolute(path));
      //MODAL_NewFolder(path);
      //MODAL_DeleteFolder(path);
      ImGui::EndPopup();
    }

    callback(path, 0u, idx);



    // Update selection state. Process outside of tree loop to avoid visual inconsistencies during the clicking-frame.
    if (node_clicked != -1) {
      if (ImGui::GetIO().KeyCtrl)
        selection_mask ^= (1 << node_clicked);          // CTRL+click to toggle
      else //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, this commented bit preserve selection when clicking on item that is part of the selection
        selection_mask = (1 << node_clicked);           // Click to single-select
    }

    //ImGui::PopStyleVar();
    
  }
}