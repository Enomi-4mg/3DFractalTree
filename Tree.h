#pragma once
#include "ofMain.h"

class Tree {
public:
    static const int MAX_DEPTH = 6;
    void setup();
    void update();
    void draw();
    void reset();

    void water(float buff);      // 長さを伸ばし、カオス度を下げる
    void fertilize(float buff);  // 太さを増し、カオス度を下げる
    void kotodama(float buff);   // カオス度を上げる

    // dayCountへのアクセサ
    int getDayCount() { return dayCount; }
    void incrementDay() { if (dayCount < 50) dayCount++; }
    float getLen() { return bLen; }

private:
    // VBO構築用の再帰関数
    void buildBranchMesh(float length, float thickness, int depth, glm::mat4 mat);
    void addStemToMesh(float r1, float r2, float h, glm::mat4 mat);
    void addFlowerToMesh(float thickness, glm::mat4 mat);
    void addLeafToMesh(float thickness, glm::mat4 mat);

    // 現在のパラメータ（描画用と目標値）
    float bLen = 0, bThick = 0, bMutation = 0;
    float tLen = 10, tThick = 2, tMutation = 0;

    int currentDepth = 0;
    int dayCount = 1;
    unsigned int seed;
    float maxMutationReached = 0;

    ofVboMesh vboMesh;
};