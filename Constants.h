// Constants.h
#pragma once
#include "ofMain.h"

struct TreeSettings {
    int maxDepth;
    float expBase, expPower;
    float lenScale, thickScale;
    float branchLenRatio, branchThickRatio;
    float baseAngle, mutationAngleMax;
    float trunkHueStart, trunkHueEnd;
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
};