// Constants.h
#pragma once
#include "ofMain.h"

// 成長タイプの定義
enum GrowthType {
    TYPE_DEFAULT,
    TYPE_ELEGANT,  // 長さ重視
    TYPE_STURDY,   // 太さ重視
    TYPE_ELDRITCH  // カオス重視
};

// 開花タイプの定義
enum FlowerType {
    FLOWER_NONE,
    FLOWER_CRYSTAL, // 雪の結晶
    FLOWER_PETAL,   // 花弁
    FLOWER_SPIRIT   // 霊魂（ヒトダマ）
};

struct TreeSettings {
    int maxDepth;
    float expBase, expPower;
    float lenScale, thickScale;
    float branchLenRatio, branchThickRatio;
    float baseAngle, mutationAngleMax;
    float trunkHueStart, trunkHueEnd;
    float twistFactor = 0.0f;
    ofColor leafColor, flowerColor;
};

struct GameState {
    int dayCount = 1;
    int skillPoints = 3;
    bool bGameEnded = false;
    bool bViewMode = false;
    bool bShowDebug = false;
    string finalTitle = "";
    int maxDays = 50;
    GrowthType currentType = TYPE_DEFAULT;
    FlowerType currentFlowerType = FLOWER_NONE;
    int resilienceLevel = 0;
};