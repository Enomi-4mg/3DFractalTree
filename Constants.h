// Constants.h
#pragma once
#include "ofMain.h"

enum GrowthType { TYPE_DEFAULT, TYPE_ELEGANT, TYPE_STURDY, TYPE_ELDRITCH };
enum FlowerType { FLOWER_NONE, FLOWER_CRYSTAL, FLOWER_PETAL, FLOWER_SPIRIT };
enum CommandType { CMD_WATER, CMD_FERTILIZER, CMD_KOTODAMA };

struct UISettings {
    string labelWater, labelFertilizer, labelKotodama;
    float btnW, btnH, btnMargin, btnBottomOffset;
    ofColor colIdle, colHover, colActive, colLocked, colText;
    float cooldownDuration;
    float statusTop, statusRight;
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
    float actionCooldown = 0.0f;
    UISettings ui;
};
