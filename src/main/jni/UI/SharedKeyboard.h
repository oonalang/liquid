#pragma once
#include "../ImGui/imgui.h"
#include "NameEntry.h"
#include <cstddef>
#include <cstring>
#include <algorithm>

namespace SharedKeyboard {
inline bool Append(char* buffer,std::size_t capacity,char value,bool profileName) {
    if (!buffer || capacity<2) return false;
    if (profileName) return NameEntry::Append(buffer,capacity,value);
    if (!NameEntry::Accepted(value)) return false;
    const std::size_t n=std::strlen(buffer);
    if (n+1>=capacity) return false;
    buffer[n]=value;
    buffer[n+1]='\0';
    return true;
}
inline void DrawInline(char* buffer,std::size_t capacity,const char* identity,
                       bool profileName,bool* open=nullptr) {
    if (!buffer || capacity<2) return;
    ImGui::PushID(identity);
    static const char* const rows[]={"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};
    const float spacing=3.f;
    const float width=ImGui::GetContentRegionAvail().x;
    const float keyWidth=std::max(1.f,(width-spacing*9.f)/10.f);
    const float height=ImGui::GetFrameHeight();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(spacing,spacing));
    for (const char* row:rows) {
        const std::size_t len=std::strlen(row);
        for (std::size_t i=0;i<len;++i) {
            char label[2]={row[i],'\0'};
            if (ImGui::Button(label,ImVec2(keyWidth,height)))
                Append(buffer,capacity,row[i],profileName);
            if (i+1<len) ImGui::SameLine();
        }
    }
    const float actionWidth=std::max(1.f,(width-spacing*2.f)/3.f);
    if (ImGui::Button("Space",ImVec2(actionWidth,height)))
        Append(buffer,capacity,' ',profileName);
    ImGui::SameLine();
    if (ImGui::Button("Del",ImVec2(actionWidth,height))) NameEntry::Backspace(buffer);
    ImGui::SameLine();
    if (ImGui::Button("Done",ImVec2(actionWidth,height)) && open) *open=false;
    ImGui::PopStyleVar();
    ImGui::PopID();
}
}
