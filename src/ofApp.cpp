#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofEnableDepthTest();
    ofEnableSmoothing();
    myTree.setup();
    ground.setup();
    // カメラの初期位置設定
    cam.setGlobalPosition(ofVec3f(0, 250, 250));
    cam.lookAt(ofVec3f(0, 0, 0)); // 木の半分くらいの高さを見つめる
    cam.setDistance(600);
    cam.setNearClip(0.1);
    cam.setTarget(ofVec3f(0, 0, 0));

    cam.disableMouseInput();
    bViewMode = false;
}

//--------------------------------------------------------------
void ofApp::update() {
    myTree.update();

    // 2. 木の成長に合わせてカメラを引くロジック
    // 木の推定全高（bLenの約4倍）を取得
    if (!bViewMode) {
        float currentTreeHeight = myTree.getLen() * 3.5f;

        // 適切な距離を計算（高さの約2.5倍程度）
        float targetDistance = max(600.0f, currentTreeHeight * 1.5f);

        // Lerpで滑らかにズームアウト
        float lerpedDistance = ofLerp(cam.getDistance(), targetDistance, 0.05f);
        cam.setDistance(lerpedDistance);

        // 注視点（Target）も木の中心（高さの半分）に合わせる
        ofVec3f currentTarget = cam.getTarget().getPosition();
        ofVec3f newTarget(0, currentTreeHeight * 0.4f, 0);
        cam.setTarget(currentTarget.getInterpolated(newTarget, 0.05f));
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(weather.getBgColor());
    cam.begin();
    ground.draw();
    myTree.draw();
    cam.end();

    // HUD表示
    if (!bViewMode) {
        ofSetColor(255);
        string hud = "Day: " + ofToString(myTree.getDayCount()) + "\n";
        hud += "Weather: " + weather.getName() + "\n";
        hud += "[1] Water [2] Fertilizer [3] Kotodama\n";
        hud += "[V] View Mode: OFF";
        ofDrawBitmapString(hud, 20, 20);
    }
    else {
        // 鑑賞モード中の操作ガイド（左下に控えめに表示）
        ofSetColor(255, 150);
        ofDrawBitmapString("VIEW MODE: ON (Press 'V' to exit)\nDrag to Rotate / Scroll to Zoom", 20, ofGetHeight() - 40);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
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
    if (!bViewMode) {
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