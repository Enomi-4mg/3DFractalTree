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

    // UI設定の読み込み
    auto ui = config["ui"];
    state.ui.labelWater = ui["labels"].value("water", "WATER");
    state.ui.labelFertilizer = ui["labels"].value("fertilizer", "FERTILIZE");
    state.ui.labelKotodama = ui["labels"].value("kotodama", "KOTODAMA");

    state.ui.colElegant.set(180, 220, 255); // 暫定。後ほどJSONから取得
    state.ui.colSturdy.set(150, 255, 100);
    state.ui.colEldritch.set(255, 50, 100);

    auto btn = ui["button"];
    state.ui.btnW = btn.value("width", 160.0f);
    state.ui.btnH = btn.value("height", 50.0f);
    state.ui.btnMargin = btn.value("margin", 20.0f);
    state.ui.btnBottomOffset = btn.value("bottom_offset", 60.0f);

    auto col = ui["colors"];
    state.ui.colIdle.set(col["idle"][0], col["idle"][1], col["idle"][2], col["idle"][3]);
    state.ui.colHover.set(col["hover"][0], col["hover"][1], col["hover"][2], col["hover"][3]);
    state.ui.colActive.set(col["active"][0], col["active"][1], col["active"][2], col["active"][3]);
    state.ui.colLocked.set(col["locked"][0], col["locked"][1], col["locked"][2], col["locked"][3]);
    state.ui.colText.set(col["text"][0], col["text"][1], col["text"][2], col["text"][3]);

    state.ui.cooldownDuration = ui.value("cooldown_time", 1.0f);
    state.ui.statusTop = ui["status_pos"].value("top", 30.0f);
    state.ui.statusRight = ui["status_pos"].value("right", 30.0f);

    // state構造体の初期化
    state.skillPoints = 3;
    state.maxDays = config["game"].value("max_days", 50);
    state.bGameEnded = false;
    state.bViewMode = false;
    state.bShowDebug = false;

    myTree.setup(config);
    lastDepthLevel = myTree.getDepthLevel();

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
    float dt = ofGetLastFrameTime();
    if (state.actionCooldown > 0) {
        state.actionCooldown -= dt;
        if (state.actionCooldown < 0) state.actionCooldown = 0;
    }
    // 終了判定
    if (myTree.getDayCount() >= state.maxDays && !state.bGameEnded) {
        state.bGameEnded = true;
        // 称号決定
        if (myTree.getLen() > 200 && myTree.getMaxMutation() < 0.3) state.finalTitle = "Elegant Giant";
        else if (myTree.getMaxMutation() > 0.8) state.finalTitle = "Herald of Chaos";
        else state.finalTitle = "Great Spirit Tree";
    }

    if (state.bShowDebug && state.bInfiniteSkills) {
        state.skillPoints = 99;
    }

    myTree.update(growthLevel, chaosResistLevel, bloomCatalystLevel, state.currentType, state.currentFlowerType);
    weather.update();

    updateCamera();

    visualDepthProgress = ofLerp(visualDepthProgress, myTree.getDepthProgress(), 0.1f);

    // パーティクル更新
    for (auto& p : particles) p.update(dt);
    ofRemove(particles, [](Particle& p) { return p.life <= 0; });
    for (auto& p : particles2D) p.update(dt);
    ofRemove(particles2D, [](Particle2D& p) { return p.life <= 0; });
    if (weather.state == RAINY && ofGetFrameNum() % 3 == 0) {
        spawn2DEffect(P_RAIN_SPLASH);
    }

    // レベルアップの検知ロジック
    int currentLvl = myTree.getDepthLevel();
    if (currentLvl > lastDepthLevel) {
        state.barState = BAR_LEVEL_UP_FLASH; // バーの発光アニメーション開始
        state.barFlashTimer = 0;
        lastDepthLevel = currentLvl;
    }
    // --- バーのアニメーション管理 ---
    if (state.barState == BAR_LEVEL_UP_FLASH) {
        state.barFlashTimer += ofGetLastFrameTime();
        if (state.barFlashTimer > 0.4f) { // 0.4秒発光を維持
            state.barState = BAR_RESET_WAIT;
            state.barFlashTimer = 0;
            state.levelUpBubbleTimer = 1.5f; // 「LEVEL UP!」表示開始
        }
    }
    else if (state.barState == BAR_RESET_WAIT) {
        visualDepthProgress = ofLerp(visualDepthProgress, 0, 0.2f);
        if (visualDepthProgress < 0.01f) {
            visualDepthProgress = 0;
            state.barState = BAR_IDLE;
        }
    }
    else {
        visualDepthProgress = ofLerp(visualDepthProgress, myTree.getDepthProgress(), 0.1f);
    }

    // --- オーラと演出タイマーの更新 ---
    if (state.auraTimer > 0) state.auraTimer -= ofGetLastFrameTime();
    if (state.levelUpBubbleTimer > 0) state.levelUpBubbleTimer -= ofGetLastFrameTime();
}

void ofApp::updateCamera() {
    if (state.bViewMode) return;

    float hFactor = config["camera"].value("height_factor", 3.5f);
    float treeH = myTree.getLen() * hFactor;
    float lerpSpeed = config["camera"].value("lerp_speed", 0.05f);
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
        float minDist = config["camera"].value("min_distance", 600.0f);
        float targetDist = std::max(minDist, treeH * 1.5f);
        float lookAtY = treeH * 0.4f;

        camAutoRotation += config["camera"].value("rotation_speed", 0.2f);
        float rad = ofDegToRad(camAutoRotation);

        glm::vec3 targetPos(sin(rad) * targetDist, lookAtY + 100, cos(rad) * targetDist);
        glm::vec3 targetLookAt(0, lookAtY, 0);

        // glm::mix (GLMのlerp) を使用してエラー回避 (修正点)
        cam.setPosition(glm::mix(cam.getPosition(), targetPos, lerpSpeed));
        cam.setTarget(glm::mix(cam.getTarget().getGlobalPosition(), targetLookAt, lerpSpeed));
        //camAutoRotation += 0.2f;
        //targetDist = max(600.0f, treeH * 1.5f);
        //lookAtY = treeH * 0.4f;

        //float rad = ofDegToRad(camAutoRotation);
        //glm::vec3 targetPos(sin(rad) * targetDist, lookAtY + 100, cos(rad) * targetDist);

        //cam.setTarget(glm::vec3(0, lookAtY, 0));
        //cam.setPosition(targetPos);
        //cam.lookAt(glm::vec3(0, lookAtY, 0));
    }
}


//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(weather.getBgFromConfig(config));

    ofEnableDepthTest();
    ofEnableLighting();
    setupLighting();

    cam.begin();
    ground.draw();
    drawAura();
    myTree.draw();
    for (auto& p : particles) p.draw();
    cam.end();

    light.disable();
    ofDisableLighting();
    ofDisableDepthTest();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (auto& p : particles2D) p.draw();
    ofDisableBlendMode();

    weather.draw2D();

    if (state.bViewMode) drawViewModeOverlay();
    else drawHUD();

    if (state.bShowDebug) drawDebugOverlay();
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

    // スキルパネル（デバッグ表示がONの時のみ）
    if (state.bShowDebug) {
        drawControlPanel();
    }

    ofPopMatrix(); // 一旦戻す（個別に座標計算するため）

    drawLeftStatusPanel(scale);  // 天候、進化印、デバッグ
    drawRightGrowthSlots(scale); // スキルボタン
    drawCenterMessage(scale);    // メッセージ、吹き出し

    drawStatusPanel();     // 右上へ
    drawBottomActionBar(); // 下部中央へ
}

string getFlowerName(FlowerType type) {
    switch (type) {
    case FLOWER_CRYSTAL: return "CRYSTAL";
    case FLOWER_PETAL: return "PETAL";
    case FLOWER_SPIRIT: return "SPIRIT";
    default: return "NONE";
    }
}


void ofApp::drawLeftStatusPanel(float scale) {
    ofPushMatrix();
    ofScale(scale, scale);
    ofTranslate(20, 20);

    // 0. 日数表示
    ofSetColor(255);
    string dayStr = "DAY: " + ofToString(myTree.getDayCount()) + " / " + ofToString(state.maxDays);
    if (state.bTimeFrozen) dayStr += " (FROZEN)";
    mainFont.drawString(dayStr, 0, 20);

    // 1. 天候バフ情報 (Y座標を 50, 70 に離す)
    ofSetColor(200, 230, 255);
    mainFont.drawString("WEATHER: " + weather.getName(), 0, 50);
    ofSetColor(255, 255, 0);
    string buff = (weather.state == SUNNY) ? "Buff: Water+" : (weather.state == RAINY) ? "Buff: Fertilizer+" : "Buff: Kotodama+";
    mainFont.drawString(buff, 0, 70);

    // 2. 進化達成の印 (Y座標 110 から開始)
    float symbolY = 110;
    auto drawSym = [&](bool active, string sym, ofColor col, string label) {
        ofSetColor(active ? col : ofColor(80));
        mainFont.drawString(sym + " " + label, 0, symbolY);
        symbolY += 30;
        };
    drawSym(state.evo.hasEvolvedType && state.currentType == TYPE_ELEGANT, "◇", state.ui.colElegant, "ELEGANT");
    drawSym(state.evo.hasEvolvedType && state.currentType == TYPE_STURDY, "□", state.ui.colSturdy, "STURDY");
    drawSym(state.evo.hasEvolvedType && state.currentType == TYPE_ELDRITCH, "◎", state.ui.colEldritch, "ELDRITCH");

    drawSym(state.evo.hasEvolvedFlower, "✿", ofColor(255, 150, 200), "BLOOM: " + getFlowerName(state.currentFlowerType));

    // 3. デバッグ設定のステータス表示
    if (state.bShowDebug) {
        ofSetColor(255, 100, 100);
        float dy = symbolY + 20;
        mainFont.drawString(">> DEBUG ACTIVE <<", 0, dy);
        mainFont.drawString(state.bInfiniteSkills ? "[P] INF SKILLS: ON" : "[P] INF SKILLS: OFF", 0, dy + 25);
        mainFont.drawString("[+] ADD EXP (Test LevelUp)", 0, dy + 50);
    }
    ofPopMatrix();
}

void ofApp::drawRightGrowthSlots(float scale) {
    float panelW = 200;
    float btnH = 60;
    float spacing = 70;

    ofPushMatrix();
    ofTranslate(ofGetWidth() - (panelW * scale) - 20, ofGetHeight() / 2 - (spacing * 1.5f) * scale);
    ofScale(scale, scale);

    hoveredSkillIndex = -1; // リセット
    // マウス座標をUI空間に変換
    float mx = (ofGetMouseX() - (ofGetWidth() - (panelW * scale) - 20)) / scale;
    float my = (ofGetMouseY() - (ofGetHeight() / 2 - (spacing * 1.5f) * scale)) / scale;

    string names[] = { "GROWTH", "RESIST", "CATALYST" };
    int levels[] = { growthLevel, chaosResistLevel, bloomCatalystLevel };
    int cost = 1;

    for (int i = 0; i < 3; i++) {
        float by = i * spacing;
        bool isHovered = (mx >= 0 && mx <= panelW && my >= by && my <= by + btnH);
        bool canAfford = (state.skillPoints >= cost);

        if (isHovered) {
            hoveredSkillIndex = i; // ホバー中のインデックスを保持
        }

        // 色の決定
        ofColor bCol = canAfford ? ofColor(100, 150, 255, 180) : ofColor(60, 150);
        if (isHovered && canAfford) bCol = ofColor(150, 200, 255, 255); // ホバー時は明るく

        ofSetColor(bCol);
        ofDrawRectRounded(0, by, panelW, btnH, 5);

        ofSetColor(255);
        mainFont.drawString(names[i], 10, by + 25);
        mainFont.drawString("LV." + ofToString(levels[i]), 10, by + 48);
        mainFont.drawString("COST: " + ofToString(cost), panelW - 80, by + 48);
    }
    // ... スキルポイント表示 ...
    ofSetColor(255, 200, 0);
    mainFont.drawString("SKILL POINTS: " + ofToString(state.skillPoints), 0, -20);
    ofPopMatrix();
}

// 画面中央のメッセージ（レベルアップ吹き出し・進化通知）を描画
void ofApp::drawCenterMessage(float scale) {
    ofPushStyle();
    ofPushMatrix();
    // 基準解像度に合わせてスケーリング
    ofScale(scale, scale);

    // 1. レベルアップの吹き出し ("LEVEL UP!")
    if (state.levelUpBubbleTimer > 0) {
        float alpha = ofMap(state.levelUpBubbleTimer, 0, 1.5, 0, 255, true);
        string msg = "LEVEL UP!";
        float tw = mainFont.stringWidth(msg);

        // 木の高さに依存せず、画面内の見やすい位置(中央より上)に固定
        float tx = (ofGetWidth() / scale) * 0.5f - tw * 0.5f;
        float ty = (ofGetHeight() / scale) * 0.35f; // 画面上部から35%の位置

        ofSetColor(255, 255, 255, alpha * 0.9);
        ofDrawRectRounded(tx - 20, ty - 30, tw + 40, 45, 10);
        ofDrawTriangle(tx + tw * 0.5f - 10, ty + 15, tx + tw * 0.5f + 10, ty + 15, tx + tw * 0.5f, ty + 35);

        ofSetColor(0, 0, 0, alpha);
        mainFont.drawString(msg, tx, ty + 5);
    }

    // 2. 進化完了メッセージ ("EVOLUTION COMPLETE")
    // Day 20 または Day 40 の当日のみ、画面中央に大きく表示
    int currentDay = myTree.getDayCount();
    if (currentDay == 20 || currentDay == 40) {
        // クールタイム中（アクション実行直後）に強調表示
        float alpha = ofMap(state.actionCooldown, 0, state.ui.cooldownDuration, 100, 255, true);

        string evoMsg = "EVOLUTION COMPLETE";
        float tw = mainFont.stringWidth(evoMsg);
        float tx = (ofGetWidth() / scale) * 0.5f - tw * 0.5f;
        float ty = (ofGetHeight() / scale) * 0.5f;

        // 文字の背後に帯状の背景
        ofSetColor(0, 0, 0, alpha * 0.6);
        ofDrawRectangle(0, ty - 40, ofGetWidth() / scale, 60);

        // 進化タイプに応じた色で強調
        ofColor evoCol = ofColor(255, 255, 0); // デフォルト
        if (currentDay == 20) {
            if (state.currentType == TYPE_ELEGANT) evoCol = state.ui.colElegant;
            else if (state.currentType == TYPE_STURDY) evoCol = state.ui.colSturdy;
            else if (state.currentType == TYPE_ELDRITCH) evoCol = state.ui.colEldritch;
        }

        ofSetColor(evoCol, alpha);
        mainFont.drawString(evoMsg, tx, ty);
    }

    ofPopMatrix();
    ofPopStyle();
}

// ステータスパネル（プログレスバー）の描画
void ofApp::drawStatusPanel() {
    float scale = getUIScale();
    float pW = 320, pH = 180;

    ofPushMatrix();
    // JSONの設定値に基づいて右上に配置
    float tx = ofGetWidth() - (pW * scale) - (state.ui.statusRight * scale);
    float ty = state.ui.statusTop * scale;
    ofTranslate(tx, ty);
    ofScale(scale, scale);

    // 背景
    ofSetColor(0, 160);
    ofDrawRectRounded(0, 0, pW, pH, 10);

    // パラメータバー描画（既存ロジックを流用）
    drawDualParamBar("Depth Level " + ofToString(myTree.getDepthLevel()), 25, 45, 270, visualDepthProgress, 0, ofColor(120, 255, 100));
    drawDualParamBar("Total Length", 25, 95, 270, ofClamp(myTree.getLen() / 400.0f, 0, 1), 0, ofColor(100, 200, 255));
    drawDualParamBar("Chaos (Cur / Max)", 25, 145, 270, myTree.getCurMutation(), myTree.getMaxMutation(), ofColor(255, 80, 150));
    ofPopMatrix();
}

void ofApp::drawBottomActionBar() {
    float scale = getUIScale();
    float btnW = state.ui.btnW;
    float btnH = state.ui.btnH;
    float margin = state.ui.btnMargin;
    int numBtns = 3;
    float totalW = (btnW * numBtns) + (margin * (numBtns - 1));

    ofPushMatrix();
    ofTranslate(ofGetWidth() / 2 - (totalW * scale) / 2, ofGetHeight() - (state.ui.btnBottomOffset * scale) - (btnH * scale));
    ofScale(scale, scale);

    string labels[] = { state.ui.labelWater, state.ui.labelFertilizer, state.ui.labelKotodama };
    CommandType types[] = { CMD_WATER, CMD_FERTILIZER, CMD_KOTODAMA };

    hoveredButtonIndex = -1;
    float mx = ofGetMouseX() / scale - (ofGetWidth() / 2 / scale - totalW / 2);
    float my = ofGetMouseY() / scale - (ofGetHeight() / scale - state.ui.btnBottomOffset - btnH);

    for (int i = 0; i < 3; i++) {
        float bx = i * (btnW + margin);
        bool isHover = (mx >= bx && mx <= bx + btnW && my >= 0 && my <= btnH);
        bool isPressed = isHover && ofGetMousePressed(); // Phase 2: 押し込み判定

        if (isHover) hoveredButtonIndex = i;

        // ボタンの沈み込み演出 (Phase 2)
        float yOff = isPressed ? state.ui.btnClickOffset : 0;

        ofSetColor(isPressed ? state.ui.colActive : (isHover ? state.ui.colHover : state.ui.colIdle));
        if (state.actionCooldown > 0) ofSetColor(state.ui.colLocked);

        ofDrawRectRounded(bx, yOff, btnW, btnH, 5);

        ofSetColor(state.ui.colText);
        mainFont.drawString(labels[i], bx + 20, btnH / 2 + 7 + yOff);

        // クールタイムゲージ
        if (state.actionCooldown > 0 && i == state.lastCommandIndex) {
            ofSetColor(state.ui.colActive);
            ofDrawRectangle(bx, btnH + yOff - 4, btnW * (state.actionCooldown / state.ui.cooldownDuration), 4);
        }
    }
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

void ofApp::drawAura() {
    if (state.auraTimer <= 0) return;

    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ADD);

    float progress = state.auraTimer / config["ui"]["aura"].value("duration", 1.5f);
    float flicker = 0.8f + 0.2f * sin(ofGetElapsedTimef() * 30.0f);

    for (auto& b : auraBeams) {
        float yAnim = fmod(ofGetElapsedTimef() * b.speed * 150.0f, 2000.0f);
        ofPushMatrix();
        ofTranslate(b.x, -yAnim + 800, b.z);

        // 90度回転させて2回描画することで十字にする
        for (int i = 0; i < 2; i++) {
            ofPushMatrix();
            ofRotateYDeg(i * 90);

            // 芯（白）
            ofSetColor(255, 255, 255, 180 * progress * flicker);
            ofDrawRectangle(-b.width * 0.1f, 0, b.width * 0.2f, b.height);

            // 外光（スキル別の色）
            ofSetColor(auraColor, 120 * progress * flicker);
            ofDrawRectangle(-b.width * 0.5f, 0, b.width, b.height);

            ofPopMatrix();
        }
        ofPopMatrix();
    }
    ofDisableBlendMode();
    ofPopStyle();
}

void ofApp::executeCommand(CommandType type) {
    // クールタイム中、またはゲーム終了後は実行不可
    if (state.actionCooldown > 0 || state.bGameEnded || state.bViewMode) return;

    state.lastCommandIndex = static_cast<int>(type);

    auto& g = config["game"];

    switch (type) {
    case CMD_WATER:
        myTree.water(1.0, state.resilienceLevel, g.value("water_increment", 15.0f));
        spawn2DEffect(P_WATER);
        break;
    case CMD_FERTILIZER:
        myTree.fertilize(1.0, state.resilienceLevel, g.value("fertilize_increment", 8.0f));
        spawn2DEffect(P_FERTILIZER);
        break;
    case CMD_KOTODAMA:
        myTree.kotodama(1.0);
        for (int i = 0; i < 40; i++) {
            Particle2D p;
            p.type = P_KOTODAMA;
            p.pos = { (float)ofGetWidth() * 0.5f, (float)ofGetHeight() * 0.5f };
            p.color = ofColor(200, 160, 255);
            p.size = 5.0f;
            p.decay = ofRandom(0.01f, 0.02f);
            p.angle = ofRandom(TWO_PI);
            p.spiralRadius = 0.0f;
            particles2D.push_back(p);
        }
        spawn2DEffect(P_KOTODAMA);
        break;
    }

    // 共通の後処理
    if (!state.bTimeFrozen) {
        myTree.incrementDay();
        checkEvolution();
        if (myTree.getDayCount() % config["game"].value("skill_interval", 5) == 0) {
            state.skillPoints++;
        }
        weather.randomize();
    }
    weather.randomize();

    // クールタイムの開始
    state.actionCooldown = state.ui.cooldownDuration;
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 'd' || key == 'D') state.bShowDebug = !state.bShowDebug;
    if (key == 'v' || key == 'V') {
        state.bViewMode = !state.bViewMode;
        state.bViewMode ? cam.enableMouseInput() : cam.disableMouseInput();
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
    if (state.bShowDebug) {
        if (key == 'w' || key == 'W') weather.randomize();
        // [T]キーで日数を一気に進める
        if (key == 't' || key == 'T') {
            for (int i = 0; i < 5; i++) {
                myTree.incrementDay();
                checkEvolution();
            }
        }
        /// [E] 成長タイプのサイクル
        if (key == 'e' || key == 'E') {
            state.currentType = static_cast<GrowthType>((state.currentType + 1) % 4);
            myTree.applyEvolution(state.currentType);
            state.evo.hasEvolvedType = true;
            state.evo.type = state.currentType;
        }
        // [F] 花の形状のサイクル修正
        if (key == 'f' || key == 'F') {
            state.currentFlowerType = static_cast<FlowerType>((state.currentFlowerType + 1) % 4);
            state.evo.hasEvolvedFlower = (state.currentFlowerType != FLOWER_NONE);
            myTree.setNeedsUpdate(); // メッシュ再構築を強制
        }
        // [P] スキル無限トグル
        if (key == 'p' || key == 'P') state.bInfiniteSkills = !state.bInfiniteSkills;
        // [Space] 時間停止トグル
        if (key == ' ') state.bTimeFrozen = !state.bTimeFrozen;
        // [+] 経験値加算（レベルアップ演出のテスト用）
        if (key == '+' || key == '=') myTree.addDebugExp(50.0f);
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

    if (key == '1') executeCommand(CMD_WATER);
    if (key == '2') executeCommand(CMD_FERTILIZER);
    if (key == '3') executeCommand(CMD_KOTODAMA);

    if (actionTaken) {
        myTree.incrementDay();
        checkEvolution();
        if (myTree.getDayCount() % config["game"].value("skill_interval", 5) == 0) {
            state.skillPoints++;
        }
        weather.randomize();
    }
}

//--------------------------------------------------------------

// --- スキル処理 ---
void ofApp::upgradeGrowth() { 
    if (state.skillPoints > 0 && growthLevel < 5) { 
        growthLevel++; state.skillPoints--;
        auraColor = state.ui.colElegant;
        state.auraTimer = config["ui"]["aura"].value("duration", 1.5f);
        auraBeams.clear();
        int count = config["ui"]["aura"].value("beam_count", 12);
        for (int i = 0; i < count; i++) {
            AuraBeam b;
            b.x = ofRandom(-200, 200);
            b.z = ofRandom(-200, 200);
            b.width = ofRandom(10, 30);
            b.speed = ofRandom(2, 5);
            b.height = ofRandom(1000, 2000);
            auraBeams.push_back(b);
        }
        state.auraTimer = config["ui"]["aura"].value("duration", 1.5f);
    } 
}
void ofApp::upgradeResist() { 
    if (state.skillPoints > 0 && chaosResistLevel < 5) { 
        chaosResistLevel++; state.skillPoints--; 
        auraColor = state.ui.colSturdy;
        state.auraTimer = config["ui"]["aura"].value("duration", 1.5f);
        auraColor = state.ui.colElegant;
        auraBeams.clear();
        int count = config["ui"]["aura"].value("beam_count", 12);
        for (int i = 0; i < count; i++) {
            AuraBeam b;
            b.x = ofRandom(-200, 200);
            b.z = ofRandom(-200, 200);
            b.width = ofRandom(10, 30);
            b.speed = ofRandom(2, 5);
            b.height = ofRandom(1000, 2000);
            auraBeams.push_back(b);
        }
        state.auraTimer = config["ui"]["aura"].value("duration", 1.5f);
    } 
}
void ofApp::upgradeCatalyst() { 
    if (state.skillPoints > 0 && bloomCatalystLevel < 5) { 
        bloomCatalystLevel++; state.skillPoints--; 
        auraColor = ofColor(255, 150, 200);
        state.auraTimer = config["ui"]["aura"].value("duration", 1.5f);
        auraBeams.clear();
        int count = config["ui"]["aura"].value("beam_count", 12);
        for (int i = 0; i < count; i++) {
            AuraBeam b;
            b.x = ofRandom(-200, 200);
            b.z = ofRandom(-200, 200);
            b.width = ofRandom(10, 30);
            b.speed = ofRandom(2, 5);
            b.height = ofRandom(1000, 2000);
            auraBeams.push_back(b);
        }
        state.auraTimer = config["ui"]["aura"].value("duration", 1.5f);
    } 
}

void ofApp::checkEvolution() {
    int day = myTree.getDayCount();
    // 20日目かつ、まだデフォルト状態の場合のみ実行
    if (day == 20 && state.currentType == TYPE_DEFAULT) {
        float L = myTree.getTotalLenEarned();
        float T = myTree.getTotalThickEarned();
        float M = myTree.getTotalMutationEarned();

        // 最大の蓄積値を持つパラメタに応じて分岐
        if (L >= T && L >= M) state.currentType = TYPE_ELEGANT;
        else if (T >= L && T >= M) state.currentType = TYPE_STURDY;
        else state.currentType = TYPE_ELDRITCH;

        // 木に進化を適用
        myTree.applyEvolution(state.currentType);
        state.evo.hasEvolvedType = true; // フラグを立てる (修正点)
        state.evo.type = state.currentType;

        // 進化演出：パーティクル放出
        spawn2DEffect(P_BLOOM);

        ofLogNotice("Evolution") << "Tree evolved into Type: " << state.currentType;
    }
    if (day == 40 && state.currentFlowerType == FLOWER_NONE) {
        // 現在の成長タイプに応じて花の形を決定
        if (state.currentType == TYPE_ELEGANT) state.currentFlowerType = FLOWER_CRYSTAL;
        else if (state.currentType == TYPE_STURDY) state.currentFlowerType = FLOWER_PETAL;
        else state.currentFlowerType = FLOWER_SPIRIT;

        state.evo.hasEvolvedFlower = true;
        spawn2DEffect(P_BLOOM);
        ofLogNotice("Evolution") << "Flower Evolved: " << state.currentFlowerType;
    }
}

void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}

void ofApp::mousePressed(int x, int y, int button) {
    if (hoveredButtonIndex != -1) {
        CommandType types[] = { CMD_WATER, CMD_FERTILIZER, CMD_KOTODAMA };
        executeCommand(types[hoveredButtonIndex]);
    }
    if (hoveredSkillIndex != -1) {
        if (hoveredSkillIndex == 0) upgradeGrowth();
        else if (hoveredSkillIndex == 1) upgradeResist();
        else if (hoveredSkillIndex == 2) upgradeCatalyst();
    }
}

void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::gotMessage(ofMessage msg) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}