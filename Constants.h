#pragma once
#include "ofMain.h"

enum GrowthType { TYPE_DEFAULT, TYPE_ELEGANT, TYPE_STURDY, TYPE_ELDRITCH };
enum FlowerType { FLOWER_NONE, FLOWER_CRYSTAL, FLOWER_PETAL, FLOWER_SPIRIT };
enum CommandType { CMD_WATER, CMD_FERTILIZER, CMD_KOTODAMA };
enum BarState { BAR_IDLE, BAR_LEVEL_UP_FLASH, BAR_RESET_WAIT };
enum ParticleType { P_WATER, P_FERTILIZER, P_KOTODAMA, P_RAIN_SPLASH, P_BLOOM };

// --- データ構造定義 ---
struct AuraBeam {
    float x, z;
    float width;
    float speed;
    float offset;
    float height;
};

struct EvolutionFlags {
    bool hasEvolvedType = false;
    bool hasEvolvedFlower = false;
    GrowthType type = TYPE_DEFAULT;
};

struct UISettings {
    string labelWater, labelFertilizer, labelKotodama;
    float btnW, btnH, btnMargin, btnBottomOffset;
    ofColor colIdle, colHover, colActive, colLocked, colText;
    ofColor colElegant, colSturdy, colEldritch;
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
    bool bTimeFrozen = false;
    bool bInfiniteSkills = false;
    string finalTitle = "";
    int maxDays = 50;
    GrowthType currentType = TYPE_DEFAULT;
    FlowerType currentFlowerType = FLOWER_NONE;
    int resilienceLevel = 0;
    float actionCooldown = 0.0f;
    UISettings ui;

    EvolutionFlags evo;
    BarState barState = BAR_IDLE;
    float barFlashTimer = 0.0f;
    float auraTimer = 0.0f;
    float levelUpBubbleTimer = 0.0f;
};