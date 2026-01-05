#pragma once
#include "ofMain.h"

class Particle {
public:
    glm::vec3 pos, vel;
    ofColor color;
    float life = 1.0f;
    float decay;

    void setup(glm::vec3 p, glm::vec3 v, ofColor c) {
        pos = p;
        vel = v;
        color = c;
        life = 1.0f;
        decay = ofRandom(0.01f, 0.03f); // 約1〜3秒で消滅
    }

    void update() {
        pos += vel;
        life -= decay;
    }

    void draw() {
        // 寿命に応じて透明度とサイズを下げる
        ofSetColor(color, life * 255);
        ofDrawSphere(pos, 2.0f * life);
    }
};