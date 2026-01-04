#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(20);
    myTree.setup();
}

//--------------------------------------------------------------
void ofApp::update() {
    myTree.update(1.0);
}

//--------------------------------------------------------------
void ofApp::draw() {
    // 3D空間の開始
    cam.begin();

    // 座標の目安として、地面にグリッドを表示（任意）
    // ofDrawGrid(100);

    myTree.draw();

    cam.end();
    // 3D空間の終了
    // 
    // UI表示（カメラの外に書くことで、画面に固定される）
    ofSetColor(255);
    ofDrawBitmapString("Keys: [F] Feed(Growth)  [M] Mutate", 20, 20);
    // エネルギー値を表示するように修正
    // ofDrawBitmapString("Energy: " + ofToString(myTree.getEnergy()), 20, 40);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 'f') myTree.feed(10);
    if (key == 'm') myTree.mutate(0.1);
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