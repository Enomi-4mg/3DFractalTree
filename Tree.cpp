#include "Tree.h"

void Tree::setup() {
    energy = 0;
    mutation = 0;
    currentDepth = 0;
    seed = ofRandom(999999);
}

void Tree::update(float weatherBonus) {
    energy += 0.5 * weatherBonus;
    currentDepth = ofClamp(floor(energy / 100.0), 0, 10);
}

void Tree::draw() {
    ofPushStyle();
    ofSetColor(200, 230, 255);
    ofSeedRandom(seed);

    ofPushMatrix();
    //ofTranslate(ofGetWidth() / 2, ofGetHeight(), 0);
    ofRotateXDeg(-90);
    drawBranch(150, currentDepth);
    ofPopMatrix();
    ofPopStyle();
}

void Tree::drawBranch(float length, int depth) {
    if (depth < 0) return;

    float jitterX = ofRandom(-30, 30) * mutation;
    float jitterY = ofRandom(-30, 30) * mutation;

    ofSetLineWidth(depth + 1);
    ofDrawLine(0, 0, 0, length, 0, 0);

    ofPushMatrix();
    ofTranslate(length, 0, 0);

    if (ofRandom(1.0) > 0.1) {
        ofPushMatrix();
        ofRotateYDeg(30 + jitterY);
        ofRotateZDeg(20 + jitterX);
        drawBranch(length * 0.75, depth - 1);
        ofPopMatrix();

        ofPushMatrix();
        ofRotateYDeg(-30 - jitterY);
        ofRotateZDeg(-20 - jitterX);
        drawBranch(length * 0.75, depth - 1);
        ofPopMatrix();
    }
    ofPopMatrix();
}

void Tree::feed(float amount) { energy += amount; }
void Tree::mutate(float amount) { mutation = ofClamp(mutation + amount, 0, 1.0); }