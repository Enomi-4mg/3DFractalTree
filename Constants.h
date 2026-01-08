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
    float btnClickOffset = 4.0f;
    ofColor colShadow = ofColor(0, 0, 0, 150);
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
    int lastCommandIndex = -1;
};

struct Particle2D {
    glm::vec2 pos, vel;
    ofColor color;
    float size, life = 1.0f, decay;
    ParticleType type;
    float angle = 0.0f;
    float spiralRadius = 0.0f;

    void update(float dt) {
        if (type == P_KOTODAMA) {
            // 中心へ吸い込まれる螺旋ロジック
            angle += 8.0f * dt;
            // 寿命(life)が減るにつれて半径を小さくし、中央へ引き寄せる
            spiralRadius = ofLerp(0, 400.0f, life);
            pos.x = ofGetWidth() * 0.5f + cos(angle) * spiralRadius;
            pos.y = ofGetHeight() * 0.5f + sin(angle) * spiralRadius;
            // 消滅直前に少し小さくする
            size = ofMap(life, 0.1, 0, 15.0f, 0, true);
        }
        else if (type == P_RAIN_SPLASH) {
            // 波紋：位置は固定で寿命だけ減らす
        }
        else {
            pos += vel;
        }
        life -= decay;
    }

    void draw() {
        if (type == P_RAIN_SPLASH) {
            // 波紋の表現を元に戻す（広がる楕円）
            ofPushStyle();
            ofNoFill();
            ofSetLineWidth(2);
            ofSetColor(color, life * 200);
            // 寿命が尽きるほど横に広がる
            float rippleScale = (1.0f - life) * size;
            ofDrawEllipse(pos, rippleScale * 2.0f, rippleScale);
            ofPopStyle();
        }
        else {
            // 通常の光る玉（背面影付き）
            ofSetColor(0, 0, 0, life * 100);
            ofDrawCircle(pos, size * 1.2f);
            ofSetColor(color, life * 255);
            ofDrawCircle(pos, size);
        }
    }
};