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

struct AudioTrack {
    float currentVol = 0.0f;
    float targetVol = 0.0f;
};

struct AudioState {
    float volume = 0.2f;      // マスター音量
    float bgmRatio = 0.7f;      // BGM比率
    float seRatio = 1.0f;       // SE比率
    float phase = 0.0f;
    float phaseStep = 0.0f;
    float targetFreq = 440.0f;
    float currentFreq = 440.0f;
    float amplitude = 0.0f;   // 現在の音量（トリガーで上昇し、自動減衰する）
    float noiseMix = 0.0f;    // 混沌度に応じたノイズ混入率
    map<string, AudioTrack> bgmTracks; // 各BGMの状態を保持
    float fadeSpeed = 1.0f;            // フェードの速さ
};

struct SigilRing {
    float radius;
    float rotation;
    float speed;
    int resolution; // 3 = 三角形, 6 = 六角形など
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

    bool bCinematicMode = false;
    float flashAlpha = 0.0f;
    int currentPresetIndex = 0;

    UISettings ui;

    EvolutionFlags evo;
    BarState barState = BAR_IDLE;
    float barFlashTimer = 0.0f;
    float auraTimer = 0.0f;
    float levelUpBubbleTimer = 0.0f;
    int lastCommandIndex = -1;
    ofColor auraColor = ofColor(255, 255, 255);

    AudioState audio;
    vector<SigilRing> sigils;

    int auraLayers = 2;
    float sigilRotationSpeed = 45.0f;
};

struct Particle2D {
    glm::vec2 pos, vel;
    ofColor color;
    float size, life = 1.0f, decay;
    ParticleType type;
    float angle = 0.0f;
    float spiralRadius = 0.0f; // 螺旋の初期半径

    void update(float dt) {
        if (type == P_KOTODAMA) {
            // 吸い込まれる螺旋ロジック
            angle += 8.0f * dt;
            // 寿命(life)が 1.0 -> 0.0 になるにつれて半径を縮小
            float currentR = spiralRadius * life;
            pos.x = ofGetWidth() * 0.5f + cos(angle) * currentR;
            pos.y = ofGetHeight() * 0.5f + sin(angle) * currentR;
            // サイズ：最初は大きく、徐々に小さく (25 -> 2)
            size = ofMap(life, 1.0f, 0.0f, 25.0f, 2.0f, true);
        }
        else if (type == P_RAIN_SPLASH) {
            // 波紋：位置固定、寿命のみ減衰
        }
        else {
            pos += vel * (dt * 60.0f);
        }
        life -= decay * (dt * 60.0f);
    }

    void draw() {
        if (type == P_RAIN_SPLASH) {
            // 復活：雨の波紋（広がる楕円）
            ofPushStyle();
            ofNoFill();
            ofSetLineWidth(2);
            ofSetColor(color, life * 150);
            float rippleW = (1.0f - life) * size * 4.0f;
            float rippleH = (1.0f - life) * size * 2.0f;
            ofDrawEllipse(pos, rippleW, rippleH);
            ofPopStyle();
        }
        else {
            // 通常の加算合成用の光る玉
            ofSetColor(0, 0, 0, life * 100); // 視認性用影
            ofDrawCircle(pos, size * 1.2f);
            ofSetColor(color, life * 255);
            ofDrawCircle(pos, size);
        }
    }
};