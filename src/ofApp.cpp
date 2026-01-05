#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofEnableDepthTest();
    ofEnableSmoothing();
    myTree.setup();
    weather.setup();
    ground.setup();

    ofEnableLighting();
    light.setup();
    // カメラの初期位置設定
    cam.setGlobalPosition(ofVec3f(0, 250, 250));
    cam.lookAt(ofVec3f(0, 0, 0)); // 木の半分くらいの高さを見つめる
    cam.setDistance(600);
    cam.setFarClip(20000);
    cam.setNearClip(0.1);
    cam.setTarget(ofVec3f(0, 0, 0));

    cam.disableMouseInput();
    bViewMode = false;
}

//--------------------------------------------------------------
void ofApp::update() {
    myTree.update();
    weather.update();

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
    ofBackground(weather.getBgColor());
    // --- 1. 3D描画（ライト有効） ---
    ofEnableLighting();

    // 天候に応じたライトのパラメータ設定
    if (weather.state == SUNNY) {
        light.setDirectional();
        light.setDiffuseColor(ofColor(255, 250, 230)); // 温かい太陽光
        light.setOrientation(glm::vec3(-45, -45, 0));
    }
    else if (weather.state == MOONLIGHT) {
        light.setPointLight();
        light.setDiffuseColor(ofColor(120, 150, 255)); // 冷たい月光
        light.setPosition(0, 500, 200);
    }
    else { // RAINY
        light.setPointLight();
        light.setDiffuseColor(ofColor(50, 60, 80)); // どんよりした暗い光
        light.setPosition(0, 800, 0);
    }

    light.enable();

    cam.begin();
    ground.draw();
    myTree.draw();
    cam.end();
    light.disable();
    ofDisableLighting();
    weather.draw2D();

    // HUD表示
    if (!bViewMode) {
        ofSetColor(255);
        string hud;
        if (myTree.getDayCount() >= 50) {
            hud = "--- GROWTH COMPLETED ---\n";
            hud += "Final Height: " + ofToString(myTree.getLen(), 1) + "\n";
            hud += "Press 'V' to enter Free Camera Mode";
        }
        else {
            hud = "Day: " + ofToString(myTree.getDayCount()) + " / 50\n";
            hud += "Weather: " + weather.getName() + "\n";
            hud += "[1] Water [2] Fertilizer [3] Kotodama\n";
            hud += "Debug: Press 'W' to Change Weather";
        }
        ofDrawBitmapString(hud, 20, 20);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    // 天気の手動切り替え（デバッグ用：'W'キー）
    if (key == 'w' || key == 'W') {
        weather.randomize();
    }
    // デバッグ用リセットボタン
    if (key == 'r' || key == 'R') {
        myTree.reset();         // 木の状態を初期化
        weather.setup();        // 雨のラインを再生成
        weather.state = SUNNY;  // 天候を晴れに戻す
        camAutoRotation = 0;    // カメラの回転位置をリセット

        // 鑑賞モードの場合は通常モードに戻す（任意）
        bViewMode = false;
        cam.disableMouseInput();
    }
    // 'V'キーで鑑賞モードを切り替え
    if (key == 'v' || key == 'V') {
        bViewMode = !bViewMode;

        if (bViewMode) {
            cam.enableMouseInput(); // マウスでの自由操作を有効化
        }
        else {
            cam.disableMouseInput(); // 自動追従に戻すため無効化
        }
    }
    // 1〜30日に応じた基本ブースト (1.0 - 2.5倍)
    if (!bViewMode && myTree.getDayCount() < 50) {
        float dayBoost = ofMap(myTree.getDayCount(), 1, 30, 1.0f, 2.5f, true);
        float weatherBonus = 1.0f;
        bool actionTaken = false;

        if (key == '1') {
            if (weather.state == SUNNY) weatherBonus = weather.getGrowthBuff();
            myTree.water(dayBoost * weatherBonus);
            actionTaken = true;
        }
        else if (key == '2') {
            if (weather.state == RAINY) weatherBonus = weather.getGrowthBuff();
            myTree.fertilize(dayBoost * weatherBonus);
            actionTaken = true;
        }
        else if (key == '3') {
            if (weather.state == MOONLIGHT) weatherBonus = weather.getGrowthBuff();
            myTree.kotodama(dayBoost * weatherBonus);
            actionTaken = true;
        }

        if (actionTaken) {
            myTree.incrementDay();
            weather.randomize();
        }
    }
}

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