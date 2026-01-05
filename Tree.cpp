#include "Tree.h"

void Tree::setup() {
    seed = ofRandom(99999);
}

void Tree::update() {
    // 全てのパラメータを滑らかに補間
    bLen = ofLerp(bLen, tLen, 0.1f);
    bThick = ofLerp(bThick, tThick, 0.1f);
    bMutation = ofLerp(bMutation, tMutation, 0.1f);

    currentDepth = ofClamp((int)(bLen / 40.0f), 0, 8);
}

void Tree::draw() {
    ofPushStyle();
    ofSeedRandom(seed); // 形状を固定ofPushMatrix();

    ofPushMatrix();
	ofTranslate(0, 0, 0); // 必要に応じて位置調整
    drawBranch(bLen, bThick, currentDepth);
    ofPopMatrix();

    ofPopStyle();
}

void Tree::water(float buff) {
    tLen += 15.0f * buff;
    tMutation = max(0.0f, tMutation - 0.05f); // 水・肥料で減少
}

void Tree::fertilize(float buff) {
    tThick += 2.0f * buff;
    tMutation = max(0.0f, tMutation - 0.05f); // 水・肥料で減少
}

void Tree::kotodama(float buff) {
    tMutation = ofClamp(tMutation + 0.1f * buff, 0.0f, 1.0f); // 言霊で上昇
}

void Tree::drawBranch(float length, float thickness, int depth) {
    if (depth < 0) {
        // 先端の装飾
        if (bMutation > 0.4 && length > 5.0) drawFlower(thickness);
        else drawLeaf();
        return;
    }

    // 色彩計算：Mutationが高いほど色が激しく変化
    float hue = ofMap(bMutation, 0, 1, 30, 200) + ofRandom(-20, 20) * bMutation;
    ofSetColor(ofColor::fromHsb(hue, 150, 100 + (length * 0.5)));

    drawStem(thickness, thickness * 0.7f, length);
    ofTranslate(0, length, 0);

    // 分岐アルゴリズム：仕様書通り先端付近(depth < 2)は2本、それ以外は3本
    int numBranches = (depth < 2) ? 2 : 3;
    float angleBase = 25.0f + (bMutation * 45.0f);

    for (int i = 0; i < numBranches; i++) {
        ofPushMatrix();
        ofRotateYDeg(i * (360.0f / numBranches));
        ofRotateZDeg(angleBase + ofRandom(-10, 10) * bMutation);
        drawBranch(length * 0.75f, thickness * 0.7f, depth - 1);
        ofPopMatrix();
    }
}

// Processing版のロジックをoFのofMeshで実装
void Tree::drawStem(float r1, float r2, float h) {
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    int segments = 5;
    for (int i = 0; i <= segments; i++) {
        float angle = i * TWO_PI / segments;
        float x = cos(angle);
        float z = sin(angle);

        mesh.addVertex(ofVec3f(x * r1, 0, z * r1));
        mesh.addVertex(ofVec3f(x * r2, h, z * r2));
    }
    mesh.draw();
}

void Tree::drawLeaf() {
    ofSetColor(60, 150, 60, 200);
    ofDrawEllipse(0, 0, 10, 20);
}

void Tree::drawFlower(float thickness) {
    ofSetColor(255, 180, 200);
    float flowerSize = thickness * 2.5f;
    ofDrawSphere(flowerSize);
}