#pragma once
#include "ofMain.h"

class Tree {
public:
    void setup();
    void update();
    void draw();

    void water(float buff);      // 長さを伸ばし、カオス度を下げる
    void fertilize(float buff);  // 太さを増し、カオス度を下げる
    void kotodama(float buff);   // カオス度を上げる

    // dayCountへのアクセサ
    int getDayCount() { return dayCount; }
    void incrementDay() { dayCount++; }
    float getLen() { return bLen; }

private:
    // 現在のパラメータ（描画用と目標値）
    float bLen = 0, bThick = 0, bMutation = 0;
    float tLen = 10, tThick = 2, tMutation = 0;

    int currentDepth = 0;
    int dayCount = 1;
    unsigned int seed;

    void drawBranch(float length, float thickness, int depth);
    void drawStem(float r1, float r2, float h);
    void drawLeaf();
    void drawFlower(float thickness);
};