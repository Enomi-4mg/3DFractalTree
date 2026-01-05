#pragma once
#include "ofMain.h"

enum WeatherState { SUNNY, RAINY, MOONLIGHT };

class Weather {
    // 2Dの雨線を管理する構造体
    struct RainLine {
        float x, y, speed, length;
    };
    vector<RainLine> rainLines;

public:
    WeatherState state = SUNNY;

    void setup() {
        rainLines.clear();
        // 画面全体に雨の線を初期配置
        for (int i = 0; i < 200; i++) {
            rainLines.push_back({ ofRandomWidth(), ofRandomHeight(), ofRandom(12, 25), ofRandom(15, 40) });
        }
    }

    void update() {
        if (state == RAINY) {
            for (auto& r : rainLines) {
                r.y += r.speed;
                if (r.y > ofGetHeight()) {
                    r.y = -r.length;
                    r.x = ofRandomWidth();
                }
            }
        }
    }

    // カメラの外（2D）で描画するメソッド
    void draw2D() {
        if (state == RAINY) {
            ofPushStyle();
            ofSetLineWidth(1);
            ofSetColor(170, 200, 255, 130); // 青白く少し透明な色
            for (auto& r : rainLines) {
                ofDrawLine(r.x, r.y, r.x, r.y + r.length);
            }
            ofPopStyle();
        }
    }

    void toggle() { state = static_cast<WeatherState>((state + 1) % 3); }
    void randomize() { state = static_cast<WeatherState>((int)ofRandom(0, 3)); }
    string getName() {
        if (state == SUNNY) return "SUNNY";
        if (state == RAINY) return "RAINY";
        return "MOONLIGHT";
    }

    ofColor getBgColor() {
        switch (state) {
        case SUNNY:     return ofColor(135, 206, 235);
        case RAINY:     return ofColor(80, 85, 95); // 少し暗めに調整
        case MOONLIGHT: return ofColor(15, 20, 40);
        default:        return ofColor(20);
        }
    }

    float getGrowthBuff() { return 1.5f; }
};