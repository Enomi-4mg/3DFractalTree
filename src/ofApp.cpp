#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofEnableDepthTest();
    ofEnableSmoothing();

    config = ofLoadJson("settings.json");
    if (config.empty() || !config.contains("ui") || !config.contains("tree")) {
        ofLogError("ofApp") << "Critical: settings.json is missing or corrupted!";
        // ここで詳細なデフォルト値をセットするか、ofSystemAlertDialogで警告を出す
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
    state.ui.colIdle = jsonToColor(col["idle"], ofColor(60, 60, 70, 200));
    state.ui.colHover = jsonToColor(col["hover"], ofColor(100, 100, 130, 255));
    state.ui.colActive = jsonToColor(col["active"], ofColor(180, 180, 220, 255));
    state.ui.colLocked = jsonToColor(col["locked"], ofColor(40, 40, 40, 150));
    state.ui.colText = jsonToColor(col["text"], ofColor(255, 255, 255, 255));

    state.ui.cooldownDuration = ui.value("cooldown_time", 1.0f);
    state.ui.statusTop = ui["status_pos"].value("top", 30.0f);
    state.ui.statusRight = ui["status_pos"].value("right", 30.0f);

    // state構造体の初期化
    state.skillPoints = 3;
    state.dayCount = 0;
    visualDepthProgress = 0;
    state.maxDays = config["game"].value("max_days", 50);
    state.bGameEnded = false;
    state.bViewMode = false;
    state.bShowDebug = false;

    state.currentPresetIndex = -1;

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

    // 音響設定のロード
    auto audioCfg = config["audio"];
    state.audio.volume = audioCfg.value("master_volume", 0.5f);
    state.audio.bgmRatio = audioCfg.value("bgm_volume_ratio", 0.7f);
    state.audio.seRatio = audioCfg.value("se_volume_ratio", 1.0f);
    state.audio.fadeSpeed = 1.0f / audioCfg["bgm"].value("fade_duration", 1.5f);

    // BGMの準備
    string keys[] = { "sunny", "rainy", "moonlight" };
    for (auto& k : keys) {
        string path = audioCfg["bgm"].value(k, "");
        if (bgmMap[k].load(path)) {
            bgmMap[k].setLoop(true);
            bgmMap[k].setVolume(0);
            bgmMap[k].play(); // 無音で流し続ける
            state.audio.bgmTracks[k] = { 0.0f, 0.0f };
        }
    }
    for (auto& pair : bgmMap) pair.second.setLoop(true);

    // SEの準備
    auto seCfg = audioCfg["se"];
    for (auto it = seCfg.begin(); it != seCfg.end(); ++it) {
        string key = it.key();
        string path = it.value();
        bool loaded = seMap[key].load(path);
        seMap[key].setLoop(false);
        seMap[key].setMultiPlay(true); // SEは重なって再生OK
        if (!loaded) ofLogError("Audio") << "Failed to load SE: " << path;
    }

    // SoundStream開始 (2ch出力, 0ch入力, 44100Hz)
    ofSoundStreamSettings settings;
    settings.setOutListener(this);
    settings.sampleRate = 44100;
    settings.numOutputChannels = 2;
    settings.numInputChannels = 0;
    soundStream.setup(settings);

    updateWeatherBGM();
}

void ofApp::updateWeatherBGM() {
    // 現在の天候状態を文字列キーに変換
    string weatherKey = "";
    if (weather.state == SUNNY) weatherKey = "sunny";
    else if (weather.state == RAINY) weatherKey = "rainy";
    else if (weather.state == MOONLIGHT) weatherKey = "moonlight";

    // 全BGMトラックの目標音量を更新
    // 現在の天候に対応するトラックのみ 1.0f、それ以外を 0.0f に設定
    for (auto& pair : state.audio.bgmTracks) {
        pair.second.targetVol = (pair.first == weatherKey) ? 1.0f : 0.0f;
    }
}

void ofApp::loadPreset(int index) {
    if (index < 0 || index >= config["presets"].size()) return;

    state.currentPresetIndex = index;
    auto p = config["presets"][index];

    // 1. 木の完全リセットと完成ロード
    myTree.setup(config); 
    myTree.reset();
    myTree.loadPresetConfig(p["tree"]); 

    // 2. 天候の反映
    string wStr = p.value("weather", "SUNNY");
    if (wStr == "SUNNY") weather.state = SUNNY;
    else if (wStr == "RAINY") weather.state = RAINY;
    else if (wStr == "MOONLIGHT") weather.state = MOONLIGHT;
    updateWeatherBGM();

    // 3. デモ状態の固定
	state.dayCount = state.maxDays - ofRandom(0, 5);
    state.bGameEnded = true; 
    visualDepthProgress = 1.0f;
    state.flashAlpha = 1.0f;

    // 4. 進化タイプ・色の反映
    state.currentType = (GrowthType)p.value("evo_type", 0);
    state.currentFlowerType = (FlowerType)p.value("flower_type", 0);
    auto col = p["aura_color"];
    state.auraColor = jsonToColor(p["aura_color"], ofColor::white);
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

    // シンセ音のエンベロープ（減衰）処理
    state.audio.amplitude *= 0.92f;
    state.audio.currentFreq = ofLerp(state.audio.currentFreq, state.audio.targetFreq, 0.1f);
    // BGMクロスフェード処理
    for (auto& pair : state.audio.bgmTracks) {
        string key = pair.first;
        AudioTrack& track = pair.second;
        track.currentVol = ofLerp(track.currentVol, track.targetVol, state.audio.fadeSpeed * dt * 5.0f);

        // マスター音量 * BGM比率 * フェード値を適用
        bgmMap[key].setVolume(track.currentVol * state.audio.volume * state.audio.bgmRatio);
    }

    updateAudioEngine(dt); // シンセ音の更新

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

    // settings.json から基本パラメータを取得
    float hFactor = config["camera"].value("height_factor", 3.5f);
    float lerpSpeed = config["camera"].value("lerp_speed", 0.05f);
    float minDist = config["camera"].value("min_distance", 600.0f);
    float rotSpeed = config["camera"].value("rotation_speed", 0.2f);

    // 木の現在の物理的長さに基づいた計算
    float currentTreeLen = myTree.getLen();
    float treeH = currentTreeLen * hFactor;

    // --- シネマティック演出用の追加計算 ---
    // 1. 成長度合いに応じた注視点の高さ調整（若いときは低く、巨木になれば中心寄りに）
    float lookAtY = treeH * ofMap(currentTreeLen, 0, 250, 0.6f, 0.4f, true);

    // 2. 距離の動的調整（成長に合わせて少し余裕を持たせる）
    float targetDist = std::max(minDist, treeH * 1.8f);

    // 3. 回転速度の自動変化（ゲーム終了後の回転を少し速くしてショーケース効果を高める）
    float actualRotSpeed = state.bGameEnded ? rotSpeed * 2.0f : rotSpeed;
    state.camAutoRotation += actualRotSpeed;

    // 4. 有機的な揺らぎ（サイン波による微細な上下運動）
    float time = ofGetElapsedTimef();
    float bobbing = sin(time * 0.5f) * (treeH * 0.05f);

    // 座標計算
    float rad = ofDegToRad(state.camAutoRotation);
    glm::vec3 targetPos(
        sin(rad) * targetDist,
        lookAtY + (treeH * 0.2f) + bobbing, // 常に少し上から見下ろす
        cos(rad) * targetDist
    );
    glm::vec3 targetLookAt(0, lookAtY, 0);

    // スムーズな補間移動
    cam.setPosition(glm::mix(cam.getPosition(), targetPos, lerpSpeed));
    cam.setTarget(glm::mix(cam.getTarget().getGlobalPosition(), targetLookAt, lerpSpeed));
}

// --------------------------------------------------------------
void ofApp::updateAudioEngine(float dt) {
    // シンセ音の振幅を減衰させる（エンベロープ処理）
    // これにより「ポーン」という打楽器的な減衰音が生まれる
    state.audio.amplitude *= 0.94f;
    if (state.audio.amplitude < 0.001f) state.audio.amplitude = 0;

    // 周波数をターゲットへ滑らかに近づける（ポルタメント効果）
    state.audio.currentFreq = ofLerp(state.audio.currentFreq, state.audio.targetFreq, 0.15f);

    // カオス度（Mutation）が高い場合はノイズ成分を増やす
    state.audio.noiseMix = ofLerp(state.audio.noiseMix, myTree.getCurMutation() * 0.5f, 0.1f);
}

// --------------------------------------------------------------
void ofApp::triggerSynthSE(float freq) {
    state.audio.targetFreq = freq;
    state.audio.amplitude = state.audio.volume; // 音量を最大にして発音開始
}

// --------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& buffer) {
    float sampleRate = 44100.0;
    for (size_t i = 0; i < buffer.getNumFrames(); i++) {
        state.audio.phaseStep = (TWO_PI * state.audio.currentFreq) / sampleRate;
        state.audio.phase += state.audio.phaseStep;

        // 純粋なサイン波
        float sineSample = sin(state.audio.phase);

        // 混沌度に応じたノイズ成分（ホワイトノイズ）
        float noiseSample = ofRandom(-1.0, 1.0);

        // ミックス
        float finalSample = (sineSample * (1.0f - state.audio.noiseMix)) + (noiseSample * state.audio.noiseMix);
        finalSample *= state.audio.amplitude;

        buffer.getSample(i, 0) = finalSample;
        buffer.getSample(i, 1) = finalSample;
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

    ofEnableAlphaBlending();
    for (auto& p : particles2D) {
        p.draw(true, weather.state);
    }
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (auto& p : particles2D) {
        p.draw(false, weather.state);
    }
    ofDisableBlendMode();

    weather.draw2D();

    if (state.bViewMode) drawViewModeOverlay();
    else if (!state.bCinematicMode) {
        drawHUD();
    }

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
    float sw = ofGetWidth();
    float sh = ofGetHeight();
    float screenScale = getUIScale();

    int count = (type == P_RAIN_SPLASH) ? 1 : 60 * screenScale;

    for (int i = 0; i < count; i++) {
        Particle2D p;
        p.type = type;

        switch (type) {
        case P_WATER: {
            p.pos = { (float)ofRandomWidth(), -20.0f };
            p.vel = { ofRandom(-2, 2) * screenScale, ofRandom(8, 15) * screenScale };
            p.color = ofColor(120, 200, 255);
            p.size = ofRandom(12, 22) * screenScale;
            p.decay = ofRandom(0.004, 0.008);
        }break;

        case P_FERTILIZER: {
            p.pos = { (float)ofRandom(sw), (float)ofRandom(sh * 0.85f, sh) };
            p.vel = { ofRandom(-4, 4), ofRandom(-8.0f, -4.0f) };
            p.color = ofColor(180, 255, 100);
            p.size = ofRandom(10, 20) * screenScale;
            p.decay = ofRandom(0.005, 0.015);
        }break;

        case P_KOTODAMA: {
            auto& k = config["effects"]["kotodama"];
            auto c = config["tree"]["colors"]["elegant"];
            p.color = ofColor(c[0], c[1], c[2]);
            p.decay = ofRandom(0.01f, 0.02f);
            p.vel = { ofRandom(-4, 4), ofRandom(-8.0f, -4.0f) };
            p.angle = ofRandom(TWO_PI);
            p.size = ofRandom(30, 105) * screenScale;
            p.spiralRadius = ofRandom(k.value("min_radius_ratio", 10.0f), k.value("max_radius_ratio", 25.0f)) * screenScale;
            p.life = 1.0f;
        }break;

        case P_RAIN_SPLASH: {
            p.pos = { ofRandomWidth(), ofRandom(ofGetHeight() * 0.8, ofGetHeight()) };
            p.vel = { 0, 0 };
            p.color = ofColor(150, 180, 255, 100);
            p.size = ofRandom(8, 16);
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

// ヘルパー関数を追加：オーラ演出をトリガーする
void ofApp::triggerAura(ofColor col) {
    state.auraColor = col;
    auraBeams.clear();

    auto& a = config["effects"]["aura"];
    float treeH = myTree.getLen() * config["camera"].value("height_factor", 3.5f);
    float effectScale = std::max(1.0f, treeH / 200.0f);

    int count = config["ui"]["aura"].value("beam_count", 15);
    for (int i = 0; i < count; i++) {
        AuraBeam b;
        b.x = ofRandom(-150, 150) * effectScale;
        b.z = ofRandom(-150, 150) * effectScale;
        b.width = ofRandom(10, 30) * effectScale;
        b.speed = ofRandom(2, 6); 
        b.height = ofRandom(treeH * 1.2f,treeH * 1.2f + 200);
        auraBeams.push_back(b);
    }
    state.auraTimer = config["ui"]["aura"].value("duration", 1.8f);
}

// 描画メソッドの修正（十字板構造）
void ofApp::drawAura() {
    if (state.auraTimer <= 0) return;

    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ADD);

    float progress = state.auraTimer / config["ui"]["aura"].value("duration", 1.8f);
    float flicker = 0.8f + 0.2f * sin(ofGetElapsedTimef() * 40.0f);

    for (auto& b : auraBeams) {
        float yAnim = fmod(ofGetElapsedTimef() * b.speed * 150.0f, 2500.0f);
        ofPushMatrix();
        ofTranslate(b.x, -yAnim + 1000, b.z);

        // 十字構造（90度回転させて2枚描画）
        for (int i = 0; i < 2; i++) {
            ofPushMatrix();
            if (i == 1) ofRotateYDeg(90);

            // 芯（白）
            ofSetColor(255, 255, 255, 150 * progress * flicker);
            ofDrawRectangle(-b.width * 0.1f, 0, b.width * 0.2f, b.height);

            // 外光（スキル別カラー）
            ofSetColor(state.auraColor, 100 * progress * flicker);
            ofDrawRectangle(-b.width * 0.5f, 0, b.width, b.height);
            ofPopMatrix();
        }
        ofPopMatrix();
    }
    ofDisableBlendMode();
    ofPopStyle();
}

void ofApp::executeCommand(CommandType type) {
    if (state.actionCooldown > 0 || state.bGameEnded || state.bViewMode) return;

    state.lastCommandIndex = static_cast<int>(type);
    auto& g = config["game"];
    float pitchBase = 440.0f + (myTree.getLen() * 0.5f);
    float seVol = state.audio.volume * state.audio.seRatio;

    switch (type) {
    case CMD_WATER:
        myTree.water(1.0, state.resilienceLevel, g.value("water_increment", 15.0f));
        spawn2DEffect(P_WATER);
        seMap["water"].setVolume(seVol);
        seMap["water"].play();
        triggerSynthSE(pitchBase);
        break;
    case CMD_FERTILIZER:
        myTree.fertilize(1.0, state.resilienceLevel, g.value("fertilize_increment", 8.0f));
        spawn2DEffect(P_FERTILIZER);
        seMap["fertilize"].setVolume(seVol);
        seMap["fertilize"].play();
        triggerSynthSE(pitchBase * 0.75f);
        break;
    case CMD_KOTODAMA:
        myTree.kotodama(1.0);
        spawn2DEffect(P_KOTODAMA);
        seMap["kotodama"].setVolume(seVol);
        seMap["kotodama"].play();
        triggerSynthSE(pitchBase * 1.5f);
        break;
    }

    // 共通の後処理
    if (!state.bTimeFrozen) {
        myTree.incrementDay();
        checkEvolution();
        if (myTree.getDayCount() % g.value("skill_interval", 5) == 0) {
            state.skillPoints++;
        }
    }

    weather.randomize();
    updateWeatherBGM();
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
        resetGame();
    }
	// 音量調整
    if (key == '[') state.audio.volume = ofClamp(state.audio.volume - 0.05f, 0, 1);
    if (key == ']') state.audio.volume = ofClamp(state.audio.volume + 0.05f, 0, 1);
    if (state.bShowDebug) {
        if (key == '4') loadPreset(0);
        else if (key >= '5' && key <= '9') loadPreset(key - '5' + 1);
        else if (key == '0') loadPreset(6);

        // シネマティックモード切替
        if (key == 'h' || key == 'H') state.bCinematicMode = !state.bCinematicMode;
        if (key == 'w' || key == 'W') {
            weather.randomize();
            updateWeatherBGM();
        }
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
    if (key == '1') executeCommand(CMD_WATER);
    if (key == '2') executeCommand(CMD_FERTILIZER);
    if (key == '3') executeCommand(CMD_KOTODAMA);
}

//--------------------------------------------------------------

// --- スキル処理 ---
void ofApp::upgradeGrowth() { 
    int cost = config["game"]["skill_costs"].value("growth", 1);
    if (state.skillPoints >= cost && growthLevel < 5) {
        growthLevel++; state.skillPoints--; 
        auto c = config["tree"]["colors"]["aura_growth"];
        triggerAura(ofColor(c[0], c[1], c[2]));
    } 
}
void ofApp::upgradeResist() {
    int cost = config["game"]["skill_costs"].value("resist", 1);
    if (state.skillPoints >= cost && chaosResistLevel < 5) { 
        chaosResistLevel++; state.skillPoints--; 
        auto c = config["tree"]["colors"]["aura_resist"];
        triggerAura(ofColor(c[0], c[1], c[2]));
    } 
}
void ofApp::upgradeCatalyst() {
    int cost = config["game"]["skill_costs"].value("catalyst", 1);
    if (state.skillPoints >= cost && bloomCatalystLevel < 5) { 
        bloomCatalystLevel++; state.skillPoints--; 
        auto c = config["tree"]["colors"]["aura_catalyst"];
        triggerAura(ofColor(c[0], c[1], c[2]));
    } 
}

void ofApp::checkEvolution() {
    auto& g = config["game"];
    int day = myTree.getDayCount();
    int dayBranch = g.value("evo_day_branch", 20);
    int dayBloom = g.value("evo_day_bloom", 40);
    // 20日目かつ、まだデフォルト状態の場合のみ実行
    if (day == dayBranch && state.currentType == TYPE_DEFAULT) {
        float L = myTree.getTotalLenEarned();
        float T = myTree.getTotalThickEarned();
        float M = myTree.getTotalMutationEarned();

        if (L >= T && L >= M) state.currentType = TYPE_ELEGANT;
        else if (T >= L && T >= M) state.currentType = TYPE_STURDY;
        else state.currentType = TYPE_ELDRITCH;

        myTree.applyEvolution(state.currentType);
        state.evo.hasEvolvedType = true;
        state.evo.type = state.currentType;
        spawn2DEffect(P_BLOOM);
    }
    if (day == dayBloom && state.currentFlowerType == FLOWER_NONE) {
        // 現在の成長タイプに応じて花の形を決定
        if (state.currentType == TYPE_ELEGANT) state.currentFlowerType = FLOWER_CRYSTAL;
        else if (state.currentType == TYPE_STURDY) state.currentFlowerType = FLOWER_PETAL;
        else state.currentFlowerType = FLOWER_SPIRIT;

        state.evo.hasEvolvedFlower = true;
        spawn2DEffect(P_BLOOM);
    }
}

void ofApp::resetGame() {
    // 1. 基本ステータスの初期化
    state.dayCount = 1;
    state.skillPoints = 3;
    state.bGameEnded = false;
    state.currentType = TYPE_DEFAULT;
    state.currentFlowerType = FLOWER_NONE;
    state.actionCooldown = 0.0f;
    state.lastCommandIndex = -1;
    state.currentPresetIndex = -1;
    state.finalTitle = "";
    state.flashAlpha = 0.0f;

    // 2. 成長段階とUIアニメーションの同期リセット
    visualDepthProgress = 0.0f;
    lastDepthLevel = 0;
    growthLevel = 0;
    chaosResistLevel = 0;
    bloomCatalystLevel = 0;
    state.barState = BAR_IDLE;
    state.barFlashTimer = 0.0f;
    state.levelUpBubbleTimer = 0.0f;

    // 3. オブジェクトの初期化
    myTree.setup(config); // 設定を再ロード
    myTree.reset();       // 木の物理パラメータを初期化
    weather.state = SUNNY;
    weather.setup();      // 雨のパーティクル等を再生成
    updateWeatherBGM();   // BGMを晴れに戻す

    // 4. 演出・エフェクトの完全消去
    particles.clear();
    particles2D.clear();
    auraBeams.clear();
    state.auraTimer = 0.0f;

    // 5. カメラとライティングのリセット
    state.camAutoRotation = 0.0f;
    cam.reset(); // easyCamの内部状態（マウス操作等）をリセット
    cam.setTarget(glm::vec3(0, 50, 0));
    cam.setDistance(600);
    cam.disableMouseInput(); // ビューモードを強制解除

    ofLogNotice("System") << "Game Reset: Returned to Day 1";
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