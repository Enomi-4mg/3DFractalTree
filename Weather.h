#pragma once
#pragma once
#include "ofMain.h"

enum WeatherState { SUNNY, RAINY, MOONLIGHT };

class Weather {
public:
    WeatherState state = SUNNY;

    // 次の天候へ切り替え
    void toggle() {
        state = static_cast<WeatherState>((state + 1) % 3);
    }

    // 天候に応じた背景色を返す
    ofColor getBgColor() {
        switch (state) {
        case SUNNY:     return ofColor(135, 206, 235); // スカイブルー
        case RAINY:     return ofColor(100, 110, 120); // グレー
        case MOONLIGHT: return ofColor(15, 20, 40);    // ミッドナイトブルー
        default:        return ofColor(20);
        }
    }

    // 成長倍率
    float getGrowthBuff() { return 1.5f; }

    // 日替わりで天候をランダムに変更する
    void randomize() {
        state = static_cast<WeatherState>((int)ofRandom(0, 3));
    }

    string getName() {
        if (state == SUNNY) return "SUNNY";
        if (state == RAINY) return "RAINY";
        return "MOONLIGHT";
    }
};