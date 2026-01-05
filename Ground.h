#pragma once
#include "ofMain.h"

class Ground {
public:
    void setup() {
        // 必要に応じてテクスチャの読み込みなどをここで行う
    }

    void draw() {
        ofPushStyle();

        // 平原の色（少し暗めの緑色）
        ofSetColor(40, 70, 40);

        ofPushMatrix();
        // openFrameworksのデフォルトではPlaneはXY平面なので、床にするためにX軸で90度回転
        ofRotateXDeg(90);

        // 十分に大きな平面を描画
        ofDrawPlane(0, 0, 4000, 4000);

        // 距離感を出すためのグリッド
        //ofSetColor(50, 80, 50);
        //ofDrawGridPlane(200, 25, false);

        ofPopMatrix();
        ofPopStyle();
    }
};