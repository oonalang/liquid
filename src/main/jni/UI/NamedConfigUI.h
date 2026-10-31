#pragma once
#include "NamedConfigStorage.h"
#include "../Features/Gameplay1657Settings.h"
#include "../Features/CustomDotCrosshair.h"
#include "../Features/HealthBarOverlay.h"
#include <array>
#include <cmath>
#include <cstring>
#include <type_traits>
#include <vector>


namespace NamedConfigUI {
using Json=NamedConfigStorage::Json;
static std::string directory;
static std::vector<std::string> profiles;
static std::string selected;
static std::string autoload;
static std::string status;
static bool initialized=false;
static bool deleteArmed=false;
static char nameBuffer[64]{};
#ifdef STUDIO_STANDALONE_PREVIEW
static std::string previewFilesDir;
#endif

static std::string AppFilesDirectory() {
#ifdef STUDIO_STANDALONE_PREVIEW
    return previewFilesDir;
#else
    if (!VM) return {};
    JNIEnv* env=nullptr;
    bool detach=false;
    const jint state=VM->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6);
    if(state==JNI_EDETACHED) {
        if(VM->AttachCurrentThread(&env,nullptr)!=JNI_OK) return {};
        detach=true;
    } else if(state!=JNI_OK || !env) return {};
    std::string result;
    jclass thread=env->FindClass("android/app/ActivityThread");
    jobject app=nullptr;
    if(thread && !env->ExceptionCheck()) {
        jmethodID method=env->GetStaticMethodID(thread,"currentApplication", "()Landroid/app/Application;");
        if(method && !env->ExceptionCheck()) app=env->CallStaticObjectMethod(thread,method);
    }
    if(env->ExceptionCheck()) env->ExceptionClear();
    if(app) {
        jclass context=env->GetObjectClass(app);
        jmethodID files=env->GetMethodID(context,"getFilesDir","()Ljava/io/File;");
        jobject folder=files?env->CallObjectMethod(app,files):nullptr;
        if(folder && !env->ExceptionCheck()) {
            jclass fileClass=env->GetObjectClass(folder);
            jmethodID absolute=env->GetMethodID(fileClass,"getAbsolutePath","()Ljava/lang/String;");
            auto path=absolute?static_cast<jstring>(env->CallObjectMethod(folder,absolute)):nullptr;
            if(path && !env->ExceptionCheck()) {
                const char* ptr=env->GetStringUTFChars(path,nullptr);
                if(ptr) {result=ptr;env->ReleaseStringUTFChars(path,ptr);}
            }
            if(path) env->DeleteLocalRef(path);
            env->DeleteLocalRef(fileClass);
            env->DeleteLocalRef(folder);
        }
        if(context) env->DeleteLocalRef(context);
        env->DeleteLocalRef(app);
    }
    if(thread) env->DeleteLocalRef(thread);
    if(env->ExceptionCheck()) env->ExceptionClear();
    if(detach) VM->DetachCurrentThread();
    return result;
#endif
}
static Json Capture() {
    Json j=Json::object();
#define SAVE_FIELD(field) j[#field] = field
    SAVE_FIELD(Config.ESPMenu.Esp);
    SAVE_FIELD(Config.ESPMenu.isPlayerLine);
    SAVE_FIELD(Config.ESPMenu.Box);
    SAVE_FIELD(Config.ESPMenu.Health);
    SAVE_FIELD(Config.ESPMenu.Name);
    SAVE_FIELD(Config.ESPMenu.Distance);
    SAVE_FIELD(Config.ESPMenu.Alert);
    SAVE_FIELD(Config.ESPMenu.Vehicles);
    SAVE_FIELD(Config.ExtraMenu.Spectate);
    SAVE_FIELD(Config.ExtraMenu.Radar1);
    SAVE_FIELD(Config.Aim.AimAssist);
    SAVE_FIELD(Config.Aim.AimAssistSize);
    SAVE_FIELD(Config.Aim.size);
    SAVE_FIELD(Config.Aim.Aimbot360);
    SAVE_FIELD(Config.Aim.AimSilent);
    SAVE_FIELD(Config.ExtraMenu.Revive);
    SAVE_FIELD(Config.ExtraMenu.Medicine);
    SAVE_FIELD(Config.ExtraMenu.Kinetic);
    SAVE_FIELD(Config.ExtraMenu.Recoil);
    SAVE_FIELD(Config.ExtraMenu.Spread);
    SAVE_FIELD(Config.ExtraMenu.Reload);
    SAVE_FIELD(Config.ExtraMenu.Scope);
    SAVE_FIELD(Config.ExtraMenu.Switch);
    SAVE_FIELD(Config.ExtraMenu.Shake);
    SAVE_FIELD(Config.ExtraMenu.Rpd);
    SAVE_FIELD(Config.ExtraMenu.Range);
    SAVE_FIELD(Config.ExtraMenu.AutoSnipe);
    SAVE_FIELD(Config.ExtraMenu.Hit);
    SAVE_FIELD(Config.ExtraMenu.Auto);
    SAVE_FIELD(Config.ExtraMenu.JetS);
    SAVE_FIELD(Config.ExtraMenu.JetJ);
    SAVE_FIELD(Config.ExtraMenu.Parachute);
    SAVE_FIELD(Config.ExtraMenu.Diving);
    SAVE_FIELD(Config.ExtraMenu.TestGulag);
    SAVE_FIELD(Config.ExtraMenu.MagnetLoot);
    SAVE_FIELD(Config.ExtraMenu.FastLoot);
    SAVE_FIELD(Config.ExtraMenu.Blueprints);
    SAVE_FIELD(Config.ExtraMenu.StretchScreen);
    SAVE_FIELD(Config.ExtraMenu.StretchAspect);
    SAVE_FIELD(Config.ExtraMenu.SpeedhackValue);
    SAVE_FIELD(SnowB);
    SAVE_FIELD(SnowBsize);
    SAVE_FIELD(SlideTackle);
    SAVE_FIELD(SlideRange);
    SAVE_FIELD(isSpeedHackEnabled);
    SAVE_FIELD(isJumpAdjustmentEnabled);
    SAVE_FIELD(jumpHeightMultiplier);
    SAVE_FIELD(isExecute);
    SAVE_FIELD(Gameplay1657::uiNoSlideFatigue);
    SAVE_FIELD(Gameplay1657::uiNoCrouch);
    SAVE_FIELD(Gameplay1657::uiNoAimDrift);
    SAVE_FIELD(Gameplay1657::uiNoHitFlinch);
    SAVE_FIELD(Gameplay1657::uiNoSprintToFireDelay);
    SAVE_FIELD(Triggerbot1657::uiEnabled);
    SAVE_FIELD(CustomDotCrosshair::settings.enabled);
    SAVE_FIELD(CustomDotCrosshair::settings.dotSize);
    SAVE_FIELD(CustomDotCrosshair::settings.opacity);
    SAVE_FIELD(CustomDotCrosshair::settings.outline);
    SAVE_FIELD(CustomDotCrosshair::settings.outlineThickness);
    SAVE_FIELD(CustomDotCrosshair::settings.centerOffsetX);
    SAVE_FIELD(CustomDotCrosshair::settings.centerOffsetY);
    SAVE_FIELD(CustomDotCrosshair::settings.firingRadius);
    SAVE_FIELD(CustomDotCrosshair::settings.showFiringZoneGuide);
    SAVE_FIELD(showFloatingIcon);
    SAVE_FIELD(Config.ESPMenu.Crosshair);
    SAVE_FIELD(Config.ESPMenu.Aimline);
    SAVE_FIELD(Config.Aim.FpsLevel);
    SAVE_FIELD(Config.Aim.FpsLevel1);
    SAVE_FIELD(Config.Aim.FpsLevelUltra);
    SAVE_FIELD(Config.Aim.FpsLevelUltra1);
    SAVE_FIELD(Config.ExtraMenu.Frame);
    SAVE_FIELD(Config.ExtraMenu.Fpss);
    SAVE_FIELD(Config.ExtraMenu.WallHack);
    SAVE_FIELD(Config.ExtraMenu.Outline);
    SAVE_FIELD(Config.ExtraMenu.Red);
    SAVE_FIELD(Config.ExtraMenu.Memento);
    SAVE_FIELD(Config.ExtraMenu.Morning);
    SAVE_FIELD(Config.ExtraMenu.Amoeba);
    SAVE_FIELD(Config.ExtraMenu.Aks);
    SAVE_FIELD(Config.ExtraMenu.Quan);
    SAVE_FIELD(Config.ExtraMenu.Glorious);
    SAVE_FIELD(Config.ExtraMenu.Neon);
    SAVE_FIELD(Config.ExtraMenu.Celestial);
    SAVE_FIELD(Config.ExtraMenu.Bam);
    SAVE_FIELD(Config.ExtraMenu.Demon);
    SAVE_FIELD(Config.ExtraMenu.Lotus);
    SAVE_FIELD(Config.ExtraMenu.Phoenix);
    SAVE_FIELD(Config.ExtraMenu.Qq);
    SAVE_FIELD(Config.ExtraMenu.Song);
    SAVE_FIELD(Config.ExtraMenu.Siege);
    SAVE_FIELD(Config.ExtraMenu.Templar);
    SAVE_FIELD(Config.ExtraMenu.Spectre);
    SAVE_FIELD(Config.ExtraMenu.Darkside);
#undef SAVE_FIELD
    j["enums.esp_type"] = static_cast<int>(Config.WIDGETS_EXPERT.EUI);
    j["enums.aim_by"] = static_cast<int>(Config.Aim.By);
    j["enums.crosshair_type"] = static_cast<int>(Config.ESPMenu.CrosshairType);
    j["enums.aim_target"] = static_cast<int>(Config.Aim.Target);
    j["enums.aim_trigger"] = static_cast<int>(Config.Aim.Trigger);
    j["enums.triggerbot_mode"] = static_cast<int>(Triggerbot1657::uiMode);
    j["enums.health_position"] = static_cast<int>(HealthBarOverlay::position);
    j["checkbox.wallhack"] = checkboxStates["wallhack"];
    j["checkbox.BRTAGS"] = checkboxStates["BRTAGS"];
    j["checkbox.AMMO"] = checkboxStates["AMMO"];
    j["checkbox.anti"] = checkboxStates["anti"];
    j["checkbox.FRAMERATE"] = checkboxStates["FRAMERATE"];
    j["checkbox.highend"] = checkboxStates["highend"];
    auto putColor=[&](const char* key,const float* v,int count) {
        if(v) {Json values=Json::array();for(int i=0;i<count;++i) values.push_back(v[i]); j[key]=std::move(values);}
    };
    putColor("ui.accent",menu,4);
    putColor("dot.color",CustomDotCrosshair::settings.color,3);
    putColor("Config.sColorsESPPLAYER.BoxPLAYER",Config.sColorsESPPLAYER.BoxPLAYER,4);
    putColor("Config.sColorsESPPLAYER.LinePLAYER",Config.sColorsESPPLAYER.LinePLAYER,4);
    putColor("Config.sColorsESPPLAYER.NamePLAYER",Config.sColorsESPPLAYER.NamePLAYER,4);
    putColor("Config.sColorsESPPLAYER.HealthPLAYER",Config.sColorsESPPLAYER.HealthPLAYER,4);
    putColor("Config.sColorsESPPLAYER.DistancePLAYER",Config.sColorsESPPLAYER.DistancePLAYER,4);
    putColor("Config.sColorsESPBOT.BoxBOT",Config.sColorsESPBOT.BoxBOT,4);
    putColor("Config.sColorsESPBOT.LineBOT",Config.sColorsESPBOT.LineBOT,4);
    putColor("Config.sColorsESPBOT.NameBOT",Config.sColorsESPBOT.NameBOT,4);
    putColor("Config.sColorsESPBOT.HealthBOT",Config.sColorsESPBOT.HealthBOT,4);
    putColor("Config.sColorsESPBOT.DistanceBOT",Config.sColorsESPBOT.DistanceBOT,4);
    return j;
}
static bool Validate(const Json& j,std::string& error) {
    if(!j.is_object()) {error="Settings object missing";return false;}
    const Json current=Capture();
    for(auto it=j.begin();it!=j.end();++it) {
        if(!current.contains(it.key())) continue; // tolerate newer versions' fields
        const auto& reference=current.at(it.key());
        if((reference.is_boolean() && !it->is_boolean()) ||
           (reference.is_number() && (!it->is_number() || !std::isfinite(it->get<double>()) ||
                std::fabs(it->get<double>())>10000.0)) ||
           (reference.is_array() && !it->is_array())) {
            error="Invalid setting: "+it.key();return false;
        }
        if(reference.is_array()) {
            if(it->size()!=reference.size()) {error="Invalid color length: "+it.key();return false;}
            for(const auto& part:*it)
                if(!part.is_number() || !std::isfinite(part.get<double>()) ||
                   part.get<double>() < 0.0 || part.get<double>() > 1.0) {
                    error="Invalid color component: "+it.key();return false;
                }
        }
    }
    return true;
}
static void Apply(const Json& j) {
#define LOAD_FIELD(field) if(j.contains(#field)) field=j.at(#field).get<std::decay_t<decltype(field)>>()
    LOAD_FIELD(Config.ESPMenu.Esp);
    LOAD_FIELD(Config.ESPMenu.isPlayerLine);
    LOAD_FIELD(Config.ESPMenu.Box);
    LOAD_FIELD(Config.ESPMenu.Health);
    LOAD_FIELD(Config.ESPMenu.Name);
    LOAD_FIELD(Config.ESPMenu.Distance);
    LOAD_FIELD(Config.ESPMenu.Alert);
    LOAD_FIELD(Config.ESPMenu.Vehicles);
    LOAD_FIELD(Config.ExtraMenu.Spectate);
    LOAD_FIELD(Config.ExtraMenu.Radar1);
    LOAD_FIELD(Config.Aim.AimAssist);
    LOAD_FIELD(Config.Aim.AimAssistSize);
    LOAD_FIELD(Config.Aim.size);
    LOAD_FIELD(Config.Aim.Aimbot360);
    LOAD_FIELD(Config.Aim.AimSilent);
    LOAD_FIELD(Config.ExtraMenu.Revive);
    LOAD_FIELD(Config.ExtraMenu.Medicine);
    LOAD_FIELD(Config.ExtraMenu.Kinetic);
    LOAD_FIELD(Config.ExtraMenu.Recoil);
    LOAD_FIELD(Config.ExtraMenu.Spread);
    LOAD_FIELD(Config.ExtraMenu.Reload);
    LOAD_FIELD(Config.ExtraMenu.Scope);
    LOAD_FIELD(Config.ExtraMenu.Switch);
    LOAD_FIELD(Config.ExtraMenu.Shake);
    LOAD_FIELD(Config.ExtraMenu.Rpd);
    LOAD_FIELD(Config.ExtraMenu.Range);
    LOAD_FIELD(Config.ExtraMenu.AutoSnipe);
    LOAD_FIELD(Config.ExtraMenu.Hit);
    LOAD_FIELD(Config.ExtraMenu.Auto);
    LOAD_FIELD(Config.ExtraMenu.JetS);
    LOAD_FIELD(Config.ExtraMenu.JetJ);
    LOAD_FIELD(Config.ExtraMenu.Parachute);
    LOAD_FIELD(Config.ExtraMenu.Diving);
    LOAD_FIELD(Config.ExtraMenu.TestGulag);
    LOAD_FIELD(Config.ExtraMenu.MagnetLoot);
    LOAD_FIELD(Config.ExtraMenu.FastLoot);
    LOAD_FIELD(Config.ExtraMenu.Blueprints);
    LOAD_FIELD(Config.ExtraMenu.StretchScreen);
    LOAD_FIELD(Config.ExtraMenu.StretchAspect);
    LOAD_FIELD(Config.ExtraMenu.SpeedhackValue);
    LOAD_FIELD(SnowB);
    LOAD_FIELD(SnowBsize);
    LOAD_FIELD(SlideTackle);
    LOAD_FIELD(SlideRange);
    LOAD_FIELD(isSpeedHackEnabled);
    LOAD_FIELD(isJumpAdjustmentEnabled);
    LOAD_FIELD(jumpHeightMultiplier);
    LOAD_FIELD(isExecute);
    LOAD_FIELD(Gameplay1657::uiNoSlideFatigue);
    LOAD_FIELD(Gameplay1657::uiNoCrouch);
    LOAD_FIELD(Gameplay1657::uiNoAimDrift);
    LOAD_FIELD(Gameplay1657::uiNoHitFlinch);
    LOAD_FIELD(Gameplay1657::uiNoSprintToFireDelay);
    LOAD_FIELD(Triggerbot1657::uiEnabled);
    LOAD_FIELD(CustomDotCrosshair::settings.enabled);
    LOAD_FIELD(CustomDotCrosshair::settings.dotSize);
    LOAD_FIELD(CustomDotCrosshair::settings.opacity);
    LOAD_FIELD(CustomDotCrosshair::settings.outline);
    LOAD_FIELD(CustomDotCrosshair::settings.outlineThickness);
    LOAD_FIELD(CustomDotCrosshair::settings.centerOffsetX);
    LOAD_FIELD(CustomDotCrosshair::settings.centerOffsetY);
    LOAD_FIELD(CustomDotCrosshair::settings.firingRadius);
    LOAD_FIELD(CustomDotCrosshair::settings.showFiringZoneGuide);
    LOAD_FIELD(showFloatingIcon);
    LOAD_FIELD(Config.ESPMenu.Crosshair);
    LOAD_FIELD(Config.ESPMenu.Aimline);
    LOAD_FIELD(Config.Aim.FpsLevel);
    LOAD_FIELD(Config.Aim.FpsLevel1);
    LOAD_FIELD(Config.Aim.FpsLevelUltra);
    LOAD_FIELD(Config.Aim.FpsLevelUltra1);
    LOAD_FIELD(Config.ExtraMenu.Frame);
    LOAD_FIELD(Config.ExtraMenu.Fpss);
    LOAD_FIELD(Config.ExtraMenu.WallHack);
    LOAD_FIELD(Config.ExtraMenu.Outline);
    LOAD_FIELD(Config.ExtraMenu.Red);
    LOAD_FIELD(Config.ExtraMenu.Memento);
    LOAD_FIELD(Config.ExtraMenu.Morning);
    LOAD_FIELD(Config.ExtraMenu.Amoeba);
    LOAD_FIELD(Config.ExtraMenu.Aks);
    LOAD_FIELD(Config.ExtraMenu.Quan);
    LOAD_FIELD(Config.ExtraMenu.Glorious);
    LOAD_FIELD(Config.ExtraMenu.Neon);
    LOAD_FIELD(Config.ExtraMenu.Celestial);
    LOAD_FIELD(Config.ExtraMenu.Bam);
    LOAD_FIELD(Config.ExtraMenu.Demon);
    LOAD_FIELD(Config.ExtraMenu.Lotus);
    LOAD_FIELD(Config.ExtraMenu.Phoenix);
    LOAD_FIELD(Config.ExtraMenu.Qq);
    LOAD_FIELD(Config.ExtraMenu.Song);
    LOAD_FIELD(Config.ExtraMenu.Siege);
    LOAD_FIELD(Config.ExtraMenu.Templar);
    LOAD_FIELD(Config.ExtraMenu.Spectre);
    LOAD_FIELD(Config.ExtraMenu.Darkside);
#undef LOAD_FIELD
    auto selectEnum=[&](const char* key,int low,int high,int fallback) {
        if(!j.contains(key)) return fallback;
        const int n=j.at(key).get<int>();
        return std::max(low,std::min(high,n));
    };
    Config.WIDGETS_EXPERT.EUI=static_cast<decltype(Config.WIDGETS_EXPERT.EUI)>(
        selectEnum("enums.esp_type",0,2,static_cast<int>(Config.WIDGETS_EXPERT.EUI)));
    Config.Aim.By=static_cast<decltype(Config.Aim.By)>(
        selectEnum("enums.aim_by",0,1,static_cast<int>(Config.Aim.By)));
    Config.ESPMenu.CrosshairType=static_cast<decltype(Config.ESPMenu.CrosshairType)>(
        selectEnum("enums.crosshair_type",0,2,static_cast<int>(Config.ESPMenu.CrosshairType)));
    Config.Aim.Target=static_cast<decltype(Config.Aim.Target)>(
        selectEnum("enums.aim_target",0,1,static_cast<int>(Config.Aim.Target)));
    Config.Aim.Trigger=static_cast<decltype(Config.Aim.Trigger)>(
        selectEnum("enums.aim_trigger",0,2,static_cast<int>(Config.Aim.Trigger)));
    Triggerbot1657::uiMode=static_cast<decltype(Triggerbot1657::uiMode)>(
        selectEnum("enums.triggerbot_mode",0,1,static_cast<int>(Triggerbot1657::uiMode)));
    HealthBarOverlay::position=static_cast<HealthBarOverlay::Position>(
        selectEnum("enums.health_position",0,1,static_cast<int>(HealthBarOverlay::position)));
    if(j.contains("checkbox.wallhack")) checkboxStates["wallhack"]=j.at("checkbox.wallhack").get<bool>();
    if(j.contains("checkbox.BRTAGS")) checkboxStates["BRTAGS"]=j.at("checkbox.BRTAGS").get<bool>();
    if(j.contains("checkbox.AMMO")) checkboxStates["AMMO"]=j.at("checkbox.AMMO").get<bool>();
    if(j.contains("checkbox.anti")) checkboxStates["anti"]=j.at("checkbox.anti").get<bool>();
    if(j.contains("checkbox.FRAMERATE")) checkboxStates["FRAMERATE"]=j.at("checkbox.FRAMERATE").get<bool>();
    if(j.contains("checkbox.highend")) checkboxStates["highend"]=j.at("checkbox.highend").get<bool>();
    auto getColor=[&](const char* key,float* out,int count) {
        if(!out || !j.contains(key)) return;
        const auto& values=j.at(key);
        if(!values.is_array()||values.size()!=static_cast<size_t>(count)) return;
        for(int i=0;i<count;++i) {
            if(!values[i].is_number()) return;
            const float value=values[i].get<float>();
            if(!std::isfinite(value)) return;
        }
        for(int i=0;i<count;++i) out[i]=std::max(0.f,std::min(1.f,values[i].get<float>()));
    };
    getColor("ui.accent",menu,4);
    getColor("dot.color",CustomDotCrosshair::settings.color,3);
    getColor("Config.sColorsESPPLAYER.BoxPLAYER",Config.sColorsESPPLAYER.BoxPLAYER,4);
    getColor("Config.sColorsESPPLAYER.LinePLAYER",Config.sColorsESPPLAYER.LinePLAYER,4);
    getColor("Config.sColorsESPPLAYER.NamePLAYER",Config.sColorsESPPLAYER.NamePLAYER,4);
    getColor("Config.sColorsESPPLAYER.HealthPLAYER",Config.sColorsESPPLAYER.HealthPLAYER,4);
    getColor("Config.sColorsESPPLAYER.DistancePLAYER",Config.sColorsESPPLAYER.DistancePLAYER,4);
    getColor("Config.sColorsESPBOT.BoxBOT",Config.sColorsESPBOT.BoxBOT,4);
    getColor("Config.sColorsESPBOT.LineBOT",Config.sColorsESPBOT.LineBOT,4);
    getColor("Config.sColorsESPBOT.NameBOT",Config.sColorsESPBOT.NameBOT,4);
    getColor("Config.sColorsESPBOT.HealthBOT",Config.sColorsESPBOT.HealthBOT,4);
    getColor("Config.sColorsESPBOT.DistanceBOT",Config.sColorsESPBOT.DistanceBOT,4);

    Config.Aim.showAimAssistSlider=Config.Aim.AimAssist;
#ifndef STUDIO_STANDALONE_PREVIEW
    SnowBshow=SnowB;
    SlideShow=SlideTackle;
    showSpeedHackSlider=isSpeedHackEnabled;
#endif
    Gameplay1657::SyncFromUi();
}
static void Refresh() {
    profiles=NamedConfigStorage::List(directory);
    if(!selected.empty() && std::find(profiles.begin(),profiles.end(),selected)==profiles.end()) selected.clear();
}
static bool LoadSelected(const std::string& name) {
    Json settings;
    std::string error;
    if(!NamedConfigStorage::Load(directory,name,settings,error) || !Validate(settings,error)) {
        status="Load failed: "+error;return false;
    }
    try {Apply(settings);} catch(const std::exception&) {status="Profile contains invalid settings";return false;}
    selected=name;
    status="Loaded: "+name;
    return true;
}
#ifdef STUDIO_STANDALONE_PREVIEW
static void SetPreviewFilesDirectory(const std::string& path) {
    previewFilesDir=path;
    initialized=false;
}
#endif
static void InitializeOnce() {
    if(initialized) return;
    initialized=true;
    const std::string appFiles=AppFilesDirectory();
    if(appFiles.empty()) {status="App files directory unavailable";return;}
    directory=appFiles+"/ethnir_configs";
    if(!NamedConfigStorage::EnsureDirectory(directory)) {status="Cannot create config folder";return;}
    Refresh();
    autoload=NamedConfigStorage::ReadAutoLoad(directory);
    if(!autoload.empty()) LoadSelected(autoload);
}
static void Draw() {
    InitializeOnce();
    if(directory.empty()) {
        ImGui::TextWrapped("%s",status.c_str());
        if(ImGui::Button("Retry storage",ImVec2(-1,38))) initialized=false;
        return;
    }
    ImGui::TextWrapped("Named profiles are stored in this app's private files directory. Only the selected autoload profile is restored at startup.");
    ImGui::Spacing();
    ImGui::TextUnformatted("CONFIG NAME");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##config_name",nameBuffer,sizeof(nameBuffer));
    if(ImGui::Button("Save as new",ImVec2(-1,38))) {
        const std::string name(nameBuffer);
        std::string error;
        if(std::find(profiles.begin(),profiles.end(),name)!=profiles.end()) status="Name already exists. Select it to overwrite.";
        else if(NamedConfigStorage::Save(directory,name,Capture(),error)) {
            selected=name;Refresh();status="Saved: "+name;
        } else status="Save failed: "+error;
    }
    ImGui::Spacing();
    ImGui::TextUnformatted("SAVED CONFIGS");
    if(ImGui::BeginChild("##profile_list",ImVec2(-1,135),true)) {
        if(profiles.empty()) ImGui::TextDisabled("No profiles saved yet.");
        for(const auto& profile:profiles) {
            const std::string label=profile+(profile==autoload?"  [AUTOLOAD]":"");
            if(ImGui::Selectable(label.c_str(),selected==profile)) {
                selected=profile;
                std::snprintf(nameBuffer,sizeof(nameBuffer),"%s",profile.c_str());
                deleteArmed=false;
            }
        }
    }
    ImGui::EndChild();
    if(!selected.empty()) {
        if(ImGui::Button("Load selected",ImVec2(-1,38))) LoadSelected(selected);
        if(ImGui::Button("Overwrite selected",ImVec2(-1,38))) {
            std::string error;
            if(NamedConfigStorage::Save(directory,selected,Capture(),error)) status="Updated: "+selected;
            else status="Save failed: "+error;
        }
        if(selected==autoload) {
            if(ImGui::Button("Disable autoload",ImVec2(-1,38))) {
                std::string error;
                if(NamedConfigStorage::SaveAutoLoad(directory,"",error)) {autoload.clear();status="Autoload disabled";}
                else status=error;
            }
        } else if(ImGui::Button("Autoload selected on startup",ImVec2(-1,38))) {
            std::string error;
            if(NamedConfigStorage::SaveAutoLoad(directory,selected,error)) {autoload=selected;status="Autoload set: "+selected;}
            else status=error;
        }
        if(ImGui::Button(deleteArmed?"Confirm delete":"Delete selected",ImVec2(-1,38))) {
            if(!deleteArmed) deleteArmed=true;
            else {
                std::string error;
                const std::string removing=selected;
                if(NamedConfigStorage::Remove(directory,removing,error)) {
                    if(removing==autoload) {
                        if(NamedConfigStorage::SaveAutoLoad(directory,"",error)) autoload.clear();
                        else status="Deleted, but could not clear autoload: "+error;
                    }
                    selected.clear();Refresh();status="Deleted: "+removing;
                } else status="Delete failed: "+error;
                deleteArmed=false;
            }
        }
    }
    ImGui::Spacing();
    if(!autoload.empty()) ImGui::TextWrapped("Autoload: %s",autoload.c_str());
    if(!status.empty()) ImGui::TextWrapped("%s",status.c_str());
}
} // namespace NamedConfigUI
