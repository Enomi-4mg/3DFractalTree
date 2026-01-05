#pragma once
#include "ofMain.h"

class Particle {
public:
    glm::vec3 pos, vel;
    ofColor color;
    bool isDead = false;

    void setup(glm::vec3 p, glm::vec3 v, ofColor c) {
        pos = p;
        vel = v;
        color = c;
        isDead = false;
    }

    void update() {
        pos += vel;
        if (pos.y < 0) isDead = true;
    }

    void draw() {
        ofSetColor(color);
        ofDrawLine(pos, pos - vel * 0.5f);
    }
};