#pragma once
#include "ofMain.h"
#include "ofxGui.h"
#include "..\Tree.h"
#include "..\Weather.h"
#include "..\Ground.h"
#include "../Particle.h"

class ofApp : public ofBaseApp{
	public:
		// --- 標準イベント ---
		void setup();
		void update();
		void draw();
		void keyPressed(int key);

		// --- 各種イベント ---
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

	private:
		// --- 内部処理の細分化 ---
		void updateCamera();          // JSONベースの自動追従カメラ
		void setupLighting();         // 天候に応じたライティング
		void drawHUD();               // プログレスバーを含むUI描画
		void drawParamBar(string label, float x, float y, float w, float ratio, ofColor col); // パラメータバー描画
		void drawDebugInfo();        // デバッグ情報描画
		void processCommand(int key); // 育成コマンド処理
		void spawnBloomParticles();   // エフェクト生成

		// --- スキル処理 ---
		void upgradeGrowth();
		void upgradeResist();
		void upgradeCatalyst();

		// --- システム変数と設定 ---
		ofJson config;
		int skillPoints = 3;
		bool bGameEnded = false;
		bool bViewMode = false;
		string finalTitle = "";
		float camAutoRotation = 0;
		bool bShowDebug = false;

		// --- オブジェクト群 ---
		Tree myTree;
		Weather weather;
		Ground ground;
		ofEasyCam cam;
		ofLight light;
		vector<Particle> particles;

		// --- GUI ---
		ofxPanel gui;
		ofParameter<int> growthLevel;
		ofParameter<int> chaosResistLevel;
		ofParameter<int> bloomCatalystLevel;
		ofxButton btnGrowth, btnResist, btnCatalyst;
};
