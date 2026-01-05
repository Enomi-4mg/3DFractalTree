#pragma once
#include "ofMain.h"

class Tree {
public:
    void setup(const ofJson& config);
    void update(int growthLevel, int chaosResist, int bloomLevel);
    void draw();
    void reset();

    void water(float buff, int growthLevel, float increment);      // 長さを伸ばし、カオス度を下げる
    void fertilize(float buff, float increment);  // 太さを増し、カオス度を下げる
    void kotodama(float buff);   // カオス度を上げる
    void incrementDay() { if (dayCount < 50) dayCount++; }

    // --- アクセサ・ユーティリティ ---
    float getLen() { return bLen; }
    float getThick() { return bThick; }
    float getMaxMutation() { return maxMutationReached; }
    float getCurMutation() { return bMutation; }
    float getDepthExp() { return depthExp; }
    int getDepthLevel() { return depthLevel; }
    int getCurrentDepth() { return currentDepth; }
    int getDayCount() { return dayCount; }
    int getSeed() { return seed; }
    float getDepthProgress();
    ofVboMesh& getVboMesh() { return vboMesh; }

private:
    // 内部ロジック：座標変換とメッシュ構築
    glm::mat4 getNextBranchMatrix(glm::mat4 tipMat, int index, int total, float angleBase);
    void buildBranchMesh(float length, float thickness, int depth, glm::mat4 mat, int chaosResist, int bloomLevel);
    void addStemToMesh(float r1, float r2, float h, glm::mat4 mat, int chaosResist,int depth);
    void addFlowerToMesh(float thickness, glm::mat4 mat);
    void addLeafToMesh(float thickness, glm::mat4 mat);
    float getExpForDepth(int d);

    // --- 育成パラメータ (b:現在値, t:目標値) ---
    float bLen = 0, bThick = 0, bMutation = 0;
    float tLen = 10, tThick = 2, tMutation = 0;
    float depthExp = 0;
    int depthLevel = 0;

    // --- 状態管理 ---
    ofVboMesh vboMesh; 
    int seed;
    int dayCount = 1;
    float maxMutationReached = 0;
    float lastMutation = 0;
    bool bNeedsUpdate = true;

    // --- JSON定数 ---
    int maxDepthConfig, currentDepth = 0;
    float depthExpBase, depthExpPower;
    float lengthVisualScale, thickVisualScale;
    float branchLenRatio, branchThickRatio;
    float baseAngle, mutationAngleMax;
    float trunkHueStart, trunkHueEnd;
    ofColor leafColor, flowerColor;
};