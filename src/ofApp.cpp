#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofEnableDepthTest();
    ofEnableSmoothing();

    config = ofLoadJson("settings.json");
    if (config.empty()) {
        ofLogError("ofApp") << "settings.json not found! Using hardcoded defaults.";
        // 必要最小限の構造をメモリ上に作成
        config["game"]["water_increment"] = 15.0;
        config["game"]["fertilize_increment"] = 8.0;
        config["camera"]["height_factor"] = 3.5;
    }

    myTree.setup(config);
    weather.setup();
    ground.setup();

    // --- GUI初期化 ---
    gui.setup("Skill & Debug", "settings.xml", 20, 150);
    gui.add(growthLevel.set("Growth Efficiency", 0, 0, 5));
    gui.add(chaosResistLevel.set("Chaos Resistance", 0, 0, 5));
    gui.add(bloomCatalystLevel.set("Bloom Catalyst", 0, 0, 5));

    btnGrowth.setup("Upgrade Growth (1pt)");
    btnResist.setup("Upgrade Resistance (1pt)");
    btnCatalyst.setup("Upgrade Bloom (1pt)");

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
    int maxDays = config["game"].value("max_days", 50);
    if (myTree.getDayCount() >= maxDays && !bGameEnded) {
        bGameEnded = true;
        // 称号決定（将来的にメソッド化も検討）
        if (myTree.getLen() > 200 && myTree.getMaxMutation() < 0.3) finalTitle = "Elegant Giant";
        else if (myTree.getMaxMutation() > 0.8) finalTitle = "Herald of Chaos";
        else finalTitle = "Great Spirit Tree";
    }

    myTree.update(growthLevel, chaosResistLevel, bloomCatalystLevel);
    weather.update();
    updateCamera();

    // パーティクルの更新と寿命管理
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->update();
        if (it->isDead) it = particles.erase(it);
        else ++it;
    }

    // 2. 木の成長に合わせてカメラを引くロジック
    // 木の推定全高（bLenの約4倍）を取得
    if (!bViewMode) {
        // --- カメラの自動ズームと回転 ---
        float currentTreeHeight = myTree.getLen() * 3.5f;
        float targetDistance = max(600.0f, currentTreeHeight * 1.5f);
        float lerpedDistance = ofLerp(cam.getDistance(), targetDistance, 0.05f);
        cam.setDistance(lerpedDistance);

        // 中心（高さの40%付近）を見つめる
        ofVec3f newTarget(0, currentTreeHeight * 0.4f, 0);

        ofVec3f currentTarget = cam.getTarget().getPosition();
        ofVec3f lerpedTarget = currentTarget.getInterpolated(newTarget, 0.05f);
        cam.setTarget(lerpedTarget);

        // 毎フレーム少しずつ回転角を増やす
        camAutoRotation += 0.2f;
        // カメラの位置を円周上で計算して更新
        float rad = ofDegToRad(camAutoRotation);
        cam.setPosition(sin(rad) * lerpedDistance, newTarget.y, cos(rad) * lerpedDistance);
        cam.lookAt(newTarget);
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(weather.getBgFromConfig(config));

    ofEnableLighting();
    setupLighting();

    cam.begin();
    ground.draw();
    myTree.draw();
    for (auto& p : particles) p.draw();
    cam.end();

    light.disable();
    ofDisableLighting();

    weather.draw2D();
    if (!bViewMode) {
        drawHUD();
    }
    else {
        ofPushStyle();
        ofSetColor(255, 200);
        ofDrawBitmapStringHighlight("VIEW MODE: Mouse to Orbit / 'V' to Return", 20, ofGetHeight() - 30, ofColor::black, ofColor::white);
        ofPopStyle();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 'd' || key == 'D') bShowDebug = !bShowDebug;
    if (bShowDebug && key == '+') {
        myTree.water(5.0, 0, 50.0);
        myTree.fertilize(5.0, 50.0);
    }
    if (key == 'w' || key == 'W') weather.randomize();
    if (key == 'r' || key == 'R') {
        myTree.reset();
        skillPoints = 3;
        growthLevel = 0;
        chaosResistLevel = 0;
        bloomCatalystLevel = 0;
        bGameEnded = false;
    }
    if (key == 'v' || key == 'V') {
        bViewMode = !bViewMode;
        bViewMode ? cam.enableMouseInput() : cam.disableMouseInput();
    }
    processCommand(key);
}

//--------------------------------------------------------------
void ofApp::updateCamera() {
    if (bViewMode) return;
    auto& c = config["camera"];
    float hFactor = c.value("height_factor", 3.5f);
    float currentTreeHeight = myTree.getLen() * hFactor;
    float targetDistance = max((float)c.value("min_distance", 600.0f), currentTreeHeight * 1.5f);

    cam.setDistance(ofLerp(cam.getDistance(), targetDistance, c.value("lerp_speed", 0.05f)));

    ofVec3f targetLookAt(0, currentTreeHeight * 0.4f, 0);
    ofVec3f currentTarget = cam.getTarget().getPosition();
    ofVec3f lerpedTarget = currentTarget.getInterpolated(targetLookAt, 0.05f);
    cam.setTarget(lerpedTarget);

    camAutoRotation += (float)c.value("rotation_speed", 0.2f);
    float rad = ofDegToRad(camAutoRotation);
    cam.setPosition(sin(rad) * cam.getDistance(), targetLookAt.y, cos(rad) * cam.getDistance());
    cam.lookAt(targetLookAt);
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
void ofApp::drawHUD() {
    int maxDays = config["game"].value("max_days", 50);

    // --- HUD背景ボックス ---
    ofPushStyle();
    ofSetColor(0, 0, 0, 120); // 半透明の黒
    ofDrawRectangle(10, 10, 350, 120); // メインHUD用
    ofDrawRectangle(10, 130, 200, 220); // GUI・スキル用
    ofPopStyle();

    gui.draw();
    // ボタン描画
    /*btnGrowth.setPosition(20, 260); btnGrowth.draw();
    btnResist.setPosition(20, 290); btnResist.draw();
    btnCatalyst.setPosition(20, 320); btnCatalyst.draw();*/
    btnGrowth.draw(); btnResist.draw(); btnCatalyst.draw();

    int panelW = 300;
    int panelX = ofGetWidth() - panelW - 20;
    int panelY = ofGetHeight() - 180;

    ofPushStyle();
    ofSetColor(0, 0, 0, 160);
    ofDrawRectRounded(panelX, panelY, panelW, 160, 8);

    // 長さ (bLen) ゲージ
    float lenRatio = ofClamp(myTree.getLen() / 500.0f, 0, 1);
    drawParamBar("Length", panelX + 20, panelY + 40, panelW - 40, lenRatio, ofColor(100, 200, 255));

    // 太さ / 次のレベルへの経験値
    float expRatio = myTree.getDepthProgress();
    drawParamBar("Depth Exp (Lv." + ofToString(myTree.getCurrentDepth()) + ")", panelX + 20, panelY + 80, panelW - 40, expRatio, ofColor(150, 255, 100));

    // 変異度 (Mutation) ゲージ
    float mutRatio = myTree.getMaxMutation();
    drawParamBar("Max Mutation", panelX + 20, panelY + 120, panelW - 40, mutRatio, ofColor(255, 100, 200));
    ofPopStyle();

    ofSetColor(255);
    ofDrawBitmapString("Skill Points: " + ofToString(skillPoints), 20, 135);

    if (bGameEnded) {
        ofDrawBitmapStringHighlight("=== GROWTH COMPLETED ===", ofGetWidth() / 2 - 100, ofGetHeight() / 2 - 20, ofColor::black, ofColor::gold);
        ofDrawBitmapStringHighlight("Title: " + finalTitle, ofGetWidth() / 2 - 100, ofGetHeight() / 2 + 10, ofColor::black, ofColor::white);
    }

    string hud = "Day: " + ofToString(myTree.getDayCount()) + " / " + ofToString(maxDays) + "\n";
    hud += "Weather: " + weather.getName() + "\n";
    hud += "[1] Water [2] Fertilizer [3] Kotodama\n";
    hud += "ViewMode: 'V' | Reset: 'R' \n Change Weather: 'W' | Debug: 'D'";
    ofDrawBitmapString(hud, 20, 20);

    // --- プログレスバー（既にある背景と色を微調整） ---
    float x = 225, y = 100, w = 120, h = 15;
    float progress = myTree.getDepthProgress();
    ofSetColor(50, 50, 50, 200);
    ofDrawRectangle(x, y, w, h);
    ofSetColor(100, 255, 100);
    ofDrawRectangle(x + 2, y + 2, (w - 4) * progress, h - 4);
    ofSetColor(255);
    ofDrawBitmapString("Next Depth: " + ofToString(int(progress * 100)) + "%", x, y - 5);
    ofDrawBitmapString("Depth Lv: " + ofToString(myTree.getCurrentDepth()), x, y + 30);

    // --- デバッグオーバーレイ ---
    if (bShowDebug) {
        ofPushStyle();
        ofSetColor(0, 255, 0);
        string dbg = "DEBUG MODE\n";
        dbg += "Tree Exp (bThick): " + ofToString(myTree.getLen(), 2) + "\n";
        dbg += "Fertilizer (bThick): " + ofToString(myTree.getDepthProgress() * 100, 1) + "%\n";
        dbg += "Mutation: " + ofToString(myTree.getMaxMutation(), 3) + "\n";
        dbg += "FPS: " + ofToString(ofGetFrameRate(), 1);
        ofDrawBitmapString(dbg, ofGetWidth() - 250, 30);
        ofPopStyle();
    }
}

void ofApp::drawParamBar(string label, float x, float y, float w, float ratio, ofColor color) {
    ofSetColor(255);
    ofDrawBitmapString(label, x, y - 5);
    ofSetColor(60);
    ofDrawRectangle(x, y, w, 12); // 背景
    ofSetColor(color);
    ofDrawRectangle(x, y, w * ratio, 12); // 中身
}

void ofApp::drawDebugInfo() {
    ofPushStyle();
    int startY = 20;
    int startX = ofGetWidth() - 250;
    ofSetColor(0, 0, 0, 200);
    ofDrawRectangle(startX - 10, startY - 10, 240, 150);

    ofSetColor(0, 255, 0);
    string d = "DEBUG INFO\n";
    d += "FPS: " + ofToString(ofGetFrameRate(), 1) + "\n";
    d += "VBO Verts: " + ofToString(myTree.getVboMesh().getNumVertices()) + "\n";
    d += "Day Count: " + ofToString(myTree.getDayCount()) + "\n";
    d += "Seed: " + ofToString(myTree.getSeed()) + "\n";
    d += "Camera Dist: " + ofToString(cam.getDistance(), 1);
    ofDrawBitmapString(d, startX, startY + 15);
    ofPopStyle();
}

void ofApp::processCommand(int key) {
    int maxDays = config["game"].value("max_days", 50);
    int skillInterval = config["game"].value("skill_interval", 5);
    
    if (bViewMode || myTree.getDayCount() >= maxDays) return;

    float dayBoost = ofMap(myTree.getDayCount(), 1, 30, 1.0f, 2.5f, true);
    float weatherBonus = 1.0f;
    bool actionTaken = false;

    auto& g = config["game"];
    // 開花判定用しきい値
    float threshold = 0.4f - (bloomCatalystLevel * 0.05f);
    bool wasBloomed = (myTree.getMaxMutation() > threshold);

    if (key == '1') {
        if (weather.state == SUNNY) weatherBonus = weather.getGrowthBuff();
        myTree.water(1.0, growthLevel, g.value("water_increment", 15.0f));
        actionTaken = true;
    }
    else if (key == '2') {
        if (weather.state == RAINY) weatherBonus = weather.getGrowthBuff();
        myTree.fertilize(1.0, g.value("fertilize_increment", 8.0f));
        actionTaken = true;
    }
    else if (key == '3') {
        if (weather.state == MOONLIGHT) weatherBonus = weather.getGrowthBuff();
        myTree.kotodama(dayBoost * weatherBonus);
        actionTaken = true;
    }

    if (actionTaken) {
        myTree.incrementDay(); 
        if (myTree.getDayCount() % skillInterval == 0) skillPoints++;

        // 実行後に開花しきい値を超えたならエフェクト
        if (!wasBloomed && myTree.getMaxMutation() > threshold) spawnBloomParticles();
        weather.randomize();
    }
}

// --- スキル処理の実装 ---
void ofApp::upgradeGrowth() { if (skillPoints > 0 && growthLevel < 5) { growthLevel++; skillPoints--; } }
void ofApp::upgradeResist() { if (skillPoints > 0 && chaosResistLevel < 5) { chaosResistLevel++; skillPoints--; } }
void ofApp::upgradeCatalyst() { if (skillPoints > 0 && bloomCatalystLevel < 5) { bloomCatalystLevel++; skillPoints--; } }


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