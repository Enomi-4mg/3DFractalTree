# **3Dフラクタル成長シミュレーター 開発計画書 Ver.5.1 (openFrameworks版)**

## **1\. プロジェクト概要**

Processing (Java) で実装された「3Dフラクタル成長シミュレーター」を openFrameworks (C++) へ移行し、描画パフォーマンスの向上と機能拡張を行う。

## **2\. 現在の進捗 (Milestone 1 完了)**

* **環境構築**: Windows 11 / Visual Studio 2022 / openFrameworks v0.12.0 の環境でビルドを確認。  
* **クラス設計**: ofApp と Tree クラスのヘッダー/ソース分離。  
* **基本機能**:  
  * 再帰による3D枝分かれ構造の描画。  
  * エネルギー蓄積による深さ（currentDepth）の増加。  
  * ofSeedRandom を利用した静的ランダム形状の保持。  
  * ofEasyCam による3D視点操作。  
  * キー入力（F/M）によるパラメータ操作（Energy, Mutation）。

## **3\. 技術仕様 (Technical Specifications)**

### **3.1 描画システム**

* **座標系**: ofEasyCam による右手座標系。  
* **描画方式**: ofDrawLine による即時モード描画（暫定）。  
* **最適化方針**: 数千の枝に対応するため ofVboMesh への移行を予定。

### **3.2 成長ロジック (Processing版からの移植待ち)**

* **エネルギー管理**: water, fertilizer に応じた長さ・太さの線形補間（lerp）。  
* **天候バフ**: SUNNY, RAINY, MOONLIGHT の3状態に応じた成長倍率（1.5倍）。  
* **色彩変異**: bHue と bMutation に基づく動的な色彩計算。

## **4\. 開発ロードマップ**

### **Phase 2: 天候システムとパラメータ補間の実装**

* Weather クラスの作成（enum による状態管理）。  
* Tree::update 内での目標値（Target）への lerp 処理。

### **Phase 3: 形状表現の拡張**

* 五角柱メッシュによる枝の描画。  
* 先端への「葉」「花」の3Dモデル（ofMesh）配置。

### **Phase 4: パフォーマンス最適化**

* 全描画要素の ofVboMesh 集約による1ドローコール化。

## **5\. 比較資料 (Processing版ソースコード抜粋)**

※移行時に参照すべきコアロジック

// 成長補間  
void update() {  
    bLen \= lerp(bLen, tLen, INTERPOLATION);  
    bThick \= lerp(bThick, tThick, INTERPOLATION);  
    bMutation \= lerp(bMutation, tMutation, INTERPOLATION);  
}

// 描画ロジック（五角柱）  
void drawStem(float r1, float r2, float h, int depth) {  
    int segments \= 5;   
    beginShape(QUAD\_STRIP);  
    for (int i \= 0; i \<= segments; i++) {  
        float angle \= i \* TWO\_PI / segments;  
        // 色彩計算ロジック  
        vertex(cos(angle) \* r1, 0, sin(angle) \* r1);  
        vertex(cos(angle) \* r2, \-h, sin(angle) \* r2);  
    }  
    endShape();  
}  
