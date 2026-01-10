#pragma once
#include "ofMain.h"
#include "..\Constants.h"

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
    // ofApp から渡された config を使って背景色を決定
    ofColor getBgFromConfig(const ofJson& config) {
        string key = "";
        if (state == SUNNY) key = "sunny_bg";
        else if (state == RAINY) key = "rainy_bg";
        else key = "moonlight_bg";

        auto c = config["weather"][key];
        return ofColor(c[0], c[1], c[2]);
    }

    float getGrowthBuff() { return 1.5f; }
};