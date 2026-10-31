#pragma once
#include <cstdint>
#include <map>
#include <string>
#include "../Features/Gameplay1657Settings.h"
#include "../Features/CustomDotCrosshair.h"

#include "../Features/Triggerbot1657Policy.h"
#include "../Features/Triggerbot1657Diagnostics.h"
namespace Triggerbot1657 {
inline bool uiEnabled = false;
inline Mode uiMode = Mode::AdsOnly;
inline void PublishUi(const ImVec2&) {}
}


struct Vector2 { float x, y; };
#include "../Hacks/Feature/Init_Hacks.h"
bool SnowB=false, isSpeedHackEnabled=false, isJumpAdjustmentEnabled=false;
float SnowBsize=0, speedHackMultiplier=1, jumpHeightMultiplier=1;
std::map<std::string,bool> checkboxStates;
bool show=true, showFloatingIcon=false;
bool HIDEESP=false;
float speed_min=.5f, speed_max=3.f;
float menu[4]={.4667f,.2667f,.9098f,1.f}; // ETHNIR NOIR: #7744E8
static float previewColors[11][4];
static void InitializePreviewState() {
    for(int i=0;i<11;++i) {
        previewColors[i][0]=i<5?1.f:0.f;
        previewColors[i][1]=i<5?0.f:1.f;
        previewColors[i][2]=0;
        previewColors[i][3]=i<5?1.f:180.f/255.f;
    }
    Config.sColorsESPPLAYER.LinePLAYER=previewColors[0];
    Config.sColorsESPPLAYER.BoxPLAYER=previewColors[1];
    Config.sColorsESPPLAYER.NamePLAYER=previewColors[2];
    Config.sColorsESPPLAYER.DistancePLAYER=previewColors[3];
    Config.sColorsESPPLAYER.HealthPLAYER=previewColors[4];
    Config.sColorsESPBOT.LineBOT=previewColors[5];
    Config.sColorsESPBOT.BoxBOT=previewColors[6];
    Config.sColorsESPBOT.NameBOT=previewColors[7];
    Config.sColorsESPBOT.HealthBOT=previewColors[8];
    Config.sColorsESPBOT.DistanceBOT=previewColors[9];
    Config.sColorsESPOTHERS.PovOTHERS=previewColors[10];
    previewColors[10][0]=225.f/255.f;
    previewColors[10][1]=previewColors[10][2]=previewColors[10][3]=1;
}
