#pragma once
#include "ofMain.h"
#include "ofxGui.h"
#include "..\Tree.h"
#include "..\Weather.h"
#include "..\Ground.h"
#include "../Particle.h"


enum ParticleType { P_WATER, P_FERTILIZER, P_KOTODAMA, P_RAIN_SPLASH, P_BLOOM };

struct Particle2D {
	glm::vec2 pos, vel;
	ofColor color;
	float size, life = 1.0, decay;
	ParticleType type;

	void update() {
		pos += vel;
		if (type == P_FERTILIZER) pos.x += sin(ofGetElapsedTimef() * 5.0) * 2.0;
		life -= decay;
	}

	void draw() {
		ofSetColor(color, life * 255);
		if (type == P_RAIN_SPLASH) {
			ofSetLineWidth(2);
			ofNoFill();
			ofDrawEllipse(pos, size * (1.0 - life) * 2.0, size * (1.0 - life)); // 広がる波紋
			ofFill();
		}
		else {
			ofDrawCircle(pos, size * (life + 0.2)); // 徐々に小さくなる円
		}
	}
};

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
		// UIコンポーネントの分割
		void drawHUD();

		void drawRightGrowthSlots(float scale);
		void drawLeftStatusPanel(float scale);
		void drawCenterMessage(float scale);

		void drawStatusPanel();
		void drawControlPanel();
		void drawViewModeOverlay();
		void drawDebugOverlay();
		ofTrueTypeFont mainFont;
		float getUIScale();
		void drawDualParamBar(string label, float x, float y, float w, float currentRatio, float maxRatio, ofColor col);
		// GUI
		void executeCommand(CommandType type);
		void drawBottomActionBar();
		int hoveredButtonIndex = -1;

		void updateCamera();
		void processCommand(int key);
		void setupLighting();
		void spawnBloomParticles();
		void spawn2DEffect(ParticleType type);

		// --- スキル処理 ---
		void upgradeGrowth();
		void upgradeResist();
		void upgradeCatalyst(); 


		void checkEvolution();

		// --- システム変数と設定 ---
		ofJson config;
		GameState state;
		float camAutoRotation = 0;
		float visualDepthProgress = 0;

		// --- オブジェクト群 ---
		Tree myTree;
		Weather weather;
		Ground ground;
		ofEasyCam cam;
		ofLight light;
		vector<Particle> particles;
		vector<Particle2D> particles2D;

		// --- GUI ---
		ofxPanel gui;
		ofParameter<int> growthLevel, chaosResistLevel, bloomCatalystLevel;
		ofxButton btnGrowth, btnResist, btnCatalyst;
};