#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofEnableDepthTest();
    ofEnableSmoothing();

    config = ofLoadJson("settings.json");
    if (config.empty()) {
        ofLogError("ofApp") << "settings.json not found! Using hardcoded defaults.";
        config["game"]["water_increment"] = 15.0;
        config["game"]["fertilize_increment"] = 8.0;
        config["camera"]["height_factor"] = 3.5;
    }
    mainFont.load("verdana.ttf", 10, true, true);

    // state構造体の初期化
    state.skillPoints = 3;
    state.maxDays = config["game"].value("max_days", 50);
    state.bGameEnded = false;
    state.bViewMode = false;
    state.bShowDebug = false;

    myTree.setup(config);
    weather.setup();
    ground.setup();

    // --- GUI初期化 ---
    gui.setup("Skill & Debug", "settings.xml", 20, 150);
    gui.add(growthLevel.set("Growth Efficiency", 0, 0, 5));
    gui.add(chaosResistLevel.set("Chaos Resistance", 0, 0, 5));
    gui.add(bloomCatalystLevel.set("Bloom Catalyst", 0, 0, 5));

    gui.add(btnGrowth.setup("Upgrade Growth (1pt)"));
    gui.add(btnResist.setup("Upgrade Resistance (1pt)"));
    gui.add(btnCatalyst.setup("Upgrade Bloom (1pt)"));

    btnGrowth.addListener(this, &ofApp::upgradeGrowth);
    btnResist.addListener(this, &ofApp::upgradeResist);
    btnCatalyst.addListener(this, &ofApp::upgradeCatalyst);

    ofEnableLighting();
    light.setup();
    cam.setDistance(400);
    cam.setGlobalPosition(ofVec3f(0, 150, 150));
    cam.lookAt(ofVec3f(0, 0, 0));
    cam.setNearClip(0.1);
    cam.setFarClip(20000);
    cam.setTarget(ofVec3f(0, 0, 0));
    cam.disableMouseInput();
}

//--------------------------------------------------------------
void ofApp::update() {
    // 終了判定
    if (myTree.getDayCount() >= state.maxDays && !state.bGameEnded) {
        state.bGameEnded = true;
        // 称号決定
        if (myTree.getLen() > 200 && myTree.getMaxMutation() < 0.3) state.finalTitle = "Elegant Giant";
        else if (myTree.getMaxMutation() > 0.8) state.finalTitle = "Herald of Chaos";
        else state.finalTitle = "Great Spirit Tree";
    }

    myTree.update(growthLevel, chaosResistLevel, bloomCatalystLevel);
    weather.update();

    updateCamera();

    visualDepthProgress = ofLerp(visualDepthProgress, myTree.getDepthProgress(), 0.1f);

    // パーティクル更新
    for (auto& p : particles) p.update();
    ofRemove(particles, [](Particle& p) { return p.life <= 0; });
    for (auto& p : particles2D) p.update();
    ofRemove(particles2D, [](Particle2D& p) { return p.life <= 0; });
    if (weather.state == RAINY && ofGetFrameNum() % 3 == 0) {
        spawn2DEffect(P_RAIN_SPLASH);
    }

    //if (bGameEnded && !bViewMode) {
    //    // 【修正】終了時のカメラ：水平視点かつ木のサイズに合わせた距離
    //    camAutoRotation += 0.3f;
    //    float centerHeight = myTree.getLen() * 1.5f; // 木の中心
    //    float targetDist = myTree.getLen() * 5.0f;   // 木のサイズに合わせた距離

    //    float rad = ofDegToRad(camAutoRotation);
    //    // y座標を centerHeight と同じにすることで水平を保つ
    //    cam.setPosition(sin(rad) * targetDist, centerHeight, cos(rad) * targetDist);
    //    cam.lookAt(glm::vec3(0, centerHeight, 0));
    //}
    //else if (!bViewMode) {
    //    // --- カメラの自動ズームと回転 ---
    //    float currentTreeHeight = myTree.getLen() * 3.5f;
    //    float targetDistance = max(600.0f, currentTreeHeight * 1.5f);
    //    float lerpedDistance = ofLerp(cam.getDistance(), targetDistance, 0.05f);
    //    cam.setDistance(lerpedDistance);

    //    // 中心（高さの40%付近）を見つめる
    //    ofVec3f newTarget(0, currentTreeHeight * 0.4f, 0);

    //    ofVec3f currentTarget = cam.getTarget().getPosition();
    //    ofVec3f lerpedTarget = currentTarget.getInterpolated(newTarget, 0.05f);
    //    cam.setTarget(lerpedTarget);

    //    // 毎フレーム少しずつ回転角を増やす
    //    camAutoRotation += 0.2f;
    //    // カメラの位置を円周上で計算して更新
    //    float rad = ofDegToRad(camAutoRotation);
    //    cam.setPosition(sin(rad) * lerpedDistance, newTarget.y, cos(rad) * lerpedDistance);
    //    cam.lookAt(newTarget);
    //}
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(weather.getBgFromConfig(config));

    ofEnableDepthTest();
    ofEnableLighting();
    setupLighting();

    cam.begin();
    ground.draw();
    myTree.draw();
    for (auto& p : particles) p.draw();
    cam.end();

    light.disable();
    ofDisableLighting();
    ofDisableDepthTest();

    for (auto& p : particles2D) p.draw();
    weather.draw2D();

    if (state.bViewMode) drawViewModeOverlay();
    else drawHUD();

    if (state.bShowDebug) drawDebugOverlay();
}
void ofApp::updateCamera() {
    if (state.bViewMode) return;

    float hFactor = config["camera"].value("height_factor", 3.5f);
    float treeH = myTree.getLen() * hFactor;
    float targetDist, lookAtY;

    if (state.bGameEnded) {
        camAutoRotation += 0.4f;
        targetDist = myTree.getLen() * 5.0f;
        lookAtY = treeH * 0.4f;

        float rad = ofDegToRad(camAutoRotation);
        cam.setPosition(sin(rad) * targetDist, lookAtY + 50, cos(rad) * targetDist);
        cam.lookAt(glm::vec3(0, lookAtY, 0));
    }
    else {
        camAutoRotation += 0.2f;
        targetDist = max(600.0f, treeH * 1.5f);
        lookAtY = treeH * 0.4f;

        float rad = ofDegToRad(camAutoRotation);
        glm::vec3 targetPos(sin(rad) * targetDist, lookAtY + 100, cos(rad) * targetDist);

        cam.setTarget(glm::vec3(0, lookAtY, 0));
        cam.setPosition(targetPos);
        cam.lookAt(glm::vec3(0, lookAtY, 0));
    }

/*    float rad = ofDegToRad(camAutoRotation);
    glm::vec3 targetPos(sin(rad) * targetDist, lookAtY + 100, cos(rad) * targetDist);

    cam.setTarget(glm::vec3(0, lookAtY, 0));
    cam.setPosition(targetPos);
    cam.lookAt(glm::vec3(0, lookAtY, 0))*/;
}

void ofApp::spawnBloomParticles() {
    for (int i = 0; i < 30; i++) {
        Particle p;
        p.setup(glm::vec3(0, myTree.getLen() * 2, 0),
            glm::vec3(ofRandom(-2, 2), ofRandom(2, 5), ofRandom(-2, 2)),
            ofColor(255, 150, 200));
        particles.push_back(p);
    }
}

//--------------------------------------------------------------
void ofApp::setupLighting() {
    // 天候ごとのライティング・プリセット
    switch (weather.state) {
    case SUNNY:
        light.setDirectional();
        light.setDiffuseColor(ofColor(255, 250, 230));
        light.setOrientation(glm::vec3(-45, -45, 0));
        break;
    case MOONLIGHT:
        light.setPointLight();
        light.setDiffuseColor(ofColor(120, 150, 255));
        light.setPosition(0, 500, 200);
        break;
    case RAINY:
        light.setPointLight();
        light.setDiffuseColor(ofColor(50, 60, 80));
        light.setPosition(0, 800, 0);
        break;
    }
    light.enable();
}

//--------------------------------------------------------------
float ofApp::getUIScale() {
    float baseW = 1024.0f; // 開発時の基準幅
    float baseH = 768.0f;  // 開発時の基準高さ
    // 縦横の比率のうち、小さい方に合わせることで画面外へのはみ出しを防ぐ
    return glm::min(ofGetWidth() / baseW, ofGetHeight() / baseH);
}

void ofApp::drawHUD() {
    float scale = getUIScale();
    ofPushMatrix();
    ofScale(scale, scale);

    ofSetColor(0, 180);
    ofDrawRectangle(15, 15, 300, 35);
    ofSetColor(255);
    mainFont.drawString("DAY: " + ofToString(myTree.getDayCount()) + " / " + ofToString(state.maxDays), 25, 40);

    drawControlPanel();
    drawStatusPanel();

    if (state.bGameEnded) {
        ofSetColor(255, 215, 0);
        string msg = "EVOLUTION COMPLETE: " + state.finalTitle;
        mainFont.drawString(msg, 512 - mainFont.stringWidth(msg) / 2, 384);
    }
    ofPopMatrix();
}

// ステータスパネル（プログレスバー）の描画
void ofApp::drawStatusPanel() {
    float scale = getUIScale();
    ofPushMatrix();
    // 右下を起点に配置
    float pW = 320, pH = 180;
    float margin = 20 * scale;;
    // 画面比率に合わせて右下を起点に配置
    ofTranslate(ofGetWidth() - (pW * scale) - margin, ofGetHeight() - (pH * scale) - margin);
    ofScale(scale, scale);
    // 背景
    ofSetColor(0, 160);
    ofDrawRectRounded(0, 0, pW, pH, 10);
    // 1. 深さ（レベル表示 + スムーズな経験値）
    string lvStr = "Depth Level " + ofToString(myTree.getDepthLevel());
    drawDualParamBar(lvStr, 25, 45, 270, visualDepthProgress, 0, ofColor(120, 255, 100));
    // 2. 木の長さ
    drawDualParamBar("Total Length", 25, 95, 270, ofClamp(myTree.getLen() / 400.0f, 0, 1), 0, ofColor(100, 200, 255));
    // 3. カオス度（現在と最大を一つのバーに統合）
    // 背景(Memory)に最大値、前面(Current)に現在値を描画
    drawDualParamBar("Chaos (Cur / Max)", 25, 145, 270, myTree.getCurMutation(), myTree.getMaxMutation(), ofColor(255, 80, 150));
    ofPopMatrix();
}

void ofApp::drawDualParamBar(string label, float x, float y, float w, float currentRatio, float maxRatio, ofColor col) {
    ofPushStyle();
    // 動的ラベル背景（文字の長さにフィット）
    float tw = mainFont.stringWidth(label) + 12;
    ofSetColor(0, 220);
    ofDrawRectangle(x - 2, y - 22, tw, 18);
    ofSetColor(255);
    mainFont.drawString(label, x + 3, y - 8);
    // バー背景
    ofSetColor(40);
    ofDrawRectangle(x, y, w, 14);
    // 最大値（記憶）の薄い表示
    if (maxRatio > 0) {
        ofSetColor(col, 60);
        ofDrawRectangle(x, y, w * maxRatio, 14);
    }
    // 現在値
    ofSetColor(col, 255);
    ofDrawRectangle(x, y, w * currentRatio, 14);
    ofPopStyle();
}

// コントロールパネルの描画
void ofApp::drawControlPanel() {
    float scale = glm::min(ofGetWidth() / 1024.0f, ofGetHeight() / 768.0f);
    ofPushStyle();
    ofPushMatrix();

    float panelW = 240; // 少し幅を広げる
    float panelH = 380; // 情報量に合わせて高くする
    ofTranslate(20 * scale, 120 * scale);
    ofScale(scale, scale);

    // 背景
    ofSetColor(0, 0, 0, 180);
    ofDrawRectRounded(0, 0, panelW, panelH, 8);

    // --- 詳細ステータス表示 ---
    ofSetColor(255);
    int ty = 25;
    mainFont.drawString("=== TREE STATUS ===", 15, ty); ty += 25;
    ofSetColor(200, 255, 200);
    mainFont.drawString("Growth Lvl:  " + ofToString(growthLevel), 15, ty); ty += 18;
    ofSetColor(200, 200, 255);
    mainFont.drawString("Resist Lvl:  " + ofToString(chaosResistLevel), 15, ty); ty += 18;
    ofSetColor(255, 200, 200);
    mainFont.drawString("Catalyst:    " + ofToString(bloomCatalystLevel), 15, ty); ty += 25;

    // 天候ボーナス表示
    ofSetColor(255, 255, 0);
    string buffInfo = "Weather Buff: ";
    if (weather.state == SUNNY) buffInfo += "Water+";
    else if (weather.state == RAINY) buffInfo += "Fertilizer+";
    else if (weather.state == MOONLIGHT) buffInfo += "Kotodama+";
    mainFont.drawString(buffInfo, 15, ty); ty += 25;

    // 残り日数
    int maxDays = config["game"].value("max_days", 50);
    int daysLeft = maxDays - myTree.getDayCount();
    ofSetColor(255);
    mainFont.drawString("Days to Limit: " + ofToString(daysLeft), 15, ty); ty += 30;

    // ofxGuiの位置調整
    gui.setPosition(30, ty);
    gui.draw();

    // コマンドガイド
    ofSetColor(200);
    string cmds = "[1] Water  [2] Fertilizer\n[3] Kotodama\n\n'V' ViewMode  'D' Debug\n'R' Reset";
    mainFont.drawString(cmds, 15, panelH - 90);

    ofPopMatrix();
    ofPopStyle();
}

void ofApp::drawDebugOverlay() {
    float scale = getUIScale();
    ofPushStyle();
    ofScale(scale, scale);
    string d = "=== DEBUG INFO ===\n";
    d += "FPS: " + ofToString(ofGetFrameRate(), 1) + "\n";
    d += "VBO Vertices: " + ofToString(myTree.getVboMesh().getNumVertices()) + "\n";
    d += "2D Particles: " + ofToString(particles2D.size()) + "\n";
    d += "3D Particles: " + ofToString(particles.size()) + "\n";
    d += "------------------\n";
    d += "Depth: " + ofToString(myTree.getDepthLevel()) + " / " + ofToString(config["tree"].value("max_depth", 8)) + "\n";
    d += "Exp: " + ofToString(myTree.getDepthExp(), 1) + "\n";
    d += "Length: " + ofToString(myTree.getLen(), 1) + " (Target: " + ofToString(myTree.getLen(), 1) + ")\n";
    d += "Thick: " + ofToString(myTree.getThick(), 1) + "\n";
    d += "Mutation: " + ofToString(myTree.getMaxMutation(), 3);
    float dw = 300;
    ofSetColor(0, 200);
    ofDrawRectangle(ofGetWidth() / scale - dw - 20, 20, dw, 190);
    ofSetColor(0, 255, 0);
    mainFont.drawString(d, ofGetWidth() / scale - dw - 10, 40);

    ofPopMatrix();
    ofPopStyle();
}

// View Modeのオーバーレイ
void ofApp::drawViewModeOverlay() {
    ofPushStyle();
    string msg = "VIEW MODE : Orbit(Mouse) / Return('V')";
    float w = msg.length() * 8 + 20;
    ofSetColor(0, 0, 0, 200);
    ofDrawRectangle(ofGetWidth() / 2 - w / 2, ofGetHeight() - 50, w, 30);
    ofSetColor(255);
    ofDrawBitmapString(msg, ofGetWidth() / 2 - w / 2 + 10, ofGetHeight() - 30);
    ofPopStyle();
}

// 演出生成の実装
void ofApp::spawn2DEffect(ParticleType type) {
    int count = (type == P_RAIN_SPLASH) ? 1 : 60;

    for (int i = 0; i < count; i++) {
        Particle2D p;
        p.type = type;

        switch (type) {
        case P_WATER: {
            p.pos = { (float)ofRandomWidth(), -20.0f };
            p.vel = { ofRandom(-3, 3), ofRandom(10, 20) };
            p.color = ofColor(120, 200, 255);
            p.size = ofRandom(3, 8);
            p.decay = ofRandom(0.01, 0.03);
        }break;

        case P_FERTILIZER: {
            p.pos = { (float)ofRandomWidth(), (float)ofGetHeight() + 20.0f };
            p.vel = { ofRandom(-4, 4), ofRandom(-8.0f, -4.0f) };
            p.color = ofColor(180, 255, 100);
            p.size = ofRandom(8, 15);
            p.decay = ofRandom(0.005, 0.015);
        }break;

        case P_KOTODAMA: {
            p.pos = { (float)ofGetWidth() / 2, (float)ofGetHeight() / 2 };
            float angle = ofRandom(TWO_PI);
            float spd = ofRandom(5, 15);
            p.vel = { cos(angle) * spd, sin(angle) * spd };
            p.color = ofColor(200, 160, 255);
            p.size = ofRandom(10, 30);
            p.decay = ofRandom(0.02, 0.04);
        }break;

        case P_RAIN_SPLASH: {
            p.pos = { ofRandomWidth(), ofRandom(ofGetHeight() * 0.8, ofGetHeight()) };
            p.vel = { 0, 0 };
            p.color = ofColor(150, 180, 255, 100);
            p.size = ofRandom(5, 15);
            p.decay = 0.05;
        }break;

        case P_BLOOM: {
            p.pos = { (float)ofGetWidth() / 2.0f + ofRandom(-150, 150),
                      (float)ofGetHeight() / 2.0f + ofRandom(-150, 150) };
            p.vel = { ofRandom(-2, 2), ofRandom(-2, 2) };
            p.color = ofColor(255, 200, 230);
            p.size = ofRandom(2, 4);
            p.decay = 0.02f;
        } break;
        }
        particles2D.push_back(p);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 'd' || key == 'D') state.bShowDebug = !state.bShowDebug;
    if (key == 'w' || key == 'W') weather.randomize();
    if (key == 'v' || key == 'V') {
        state.bViewMode = !state.bViewMode;
        state.bViewMode ? cam.enableMouseInput() : cam.disableMouseInput();
    }
    if (state.bShowDebug && key == '+') {
        myTree.water(5.0, 0, 50.0);
        myTree.fertilize(5.0, 50.0);
    }
    if (key == 'r' || key == 'R') {
        myTree.reset();
        particles.clear();
        particles2D.clear();
        state.skillPoints = 3;
        growthLevel = 0;
        chaosResistLevel = 0;
        bloomCatalystLevel = 0;
        state.bGameEnded = false;
    }
    processCommand(key);
}

void ofApp::processCommand(int key) {
    if (state.bViewMode || myTree.getDayCount() >= state.maxDays) return;

    bool actionTaken = false;
    auto& g = config["game"];

    // 開花判定用しきい値
    float threshold = 0.4f - (bloomCatalystLevel * 0.05f);
    bool wasBloomed = (myTree.getMaxMutation() > threshold);

    if (key == '1') {
        myTree.water(1.0, growthLevel, g.value("water_increment", 15.0f));
        spawn2DEffect(P_WATER);
        actionTaken = true;
    }
    else if (key == '2') {
        myTree.fertilize(1.0, g.value("fertilize_increment", 8.0f));
        spawn2DEffect(P_FERTILIZER);
        actionTaken = true;
    }
    else if (key == '3') {
        myTree.kotodama(1.0);
        spawn2DEffect(P_KOTODAMA);
        actionTaken = true;
    }

    if (actionTaken) {
        myTree.incrementDay();
        if (myTree.getDayCount() % config["game"].value("skill_interval", 5) == 0) {
            state.skillPoints++;
        }
        weather.randomize();
    }
}

//--------------------------------------------------------------

// --- スキル処理 ---
void ofApp::upgradeGrowth() { if (state.skillPoints > 0 && growthLevel < 5) { growthLevel++; state.skillPoints--; } }
void ofApp::upgradeResist() { if (state.skillPoints > 0 && chaosResistLevel < 5) { chaosResistLevel++; state.skillPoints--; } }
void ofApp::upgradeCatalyst() { if (state.skillPoints > 0 && bloomCatalystLevel < 5) { bloomCatalystLevel++; state.skillPoints--; } }

void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mousePressed(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::gotMessage(ofMessage msg) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}