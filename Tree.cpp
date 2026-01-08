#include "Tree.h"
void Tree::setup(const ofJson& config) {
    seed = ofRandom(99999);
    auto t = config["tree"];
    s.maxDepth = t.value("max_depth", 6);
    s.expBase = t.value("depth_exp_base", 30.0f);
    s.expPower = t.value("depth_exp_power", 1.6f);
    s.lenScale = t.value("length_visual_scale", 1.5f);
    s.thickScale = t.value("thickness_visual_scale", 0.8f);
    s.branchLenRatio = t.value("branch_length_ratio", 0.75f);
    s.branchThickRatio = t.value("branch_thick_ratio", 0.7f);
    s.baseAngle = t.value("base_angle", 25.0f);
    s.mutationAngleMax = t.value("mutation_angle_max", 45.0f);

    auto c = t["colors"];
    s.trunkHueStart = c.value("trunk_hue_start", 20.0f);
    s.trunkHueEnd = c.value("trunk_hue_end", 160.0f);
    s.leafColor = ofColor(c["leaf"][0], c["leaf"][1], c["leaf"][2], c["leaf"][3]);

	// test value
    //s.twistFactor = 90.0f;
}

void Tree::update(int growthLevel, int chaosResist, int bloomLevel, GrowthType gType, FlowerType fType) {
    // 補間ロジックは維持
    bLen = ofLerp(bLen, tLen, 0.1f);
    bThick = ofLerp(bThick, tThick, 0.1f);
    bMutation = ofLerp(bMutation, tMutation, 0.1f);

    maxMutationReached = max(maxMutationReached, bMutation);

    if (depthLevel < s.maxDepth && depthExp >= getExpForDepth(depthLevel + 1)) {
        depthLevel++;
        bNeedsUpdate = true;
    }

    if (bNeedsUpdate || abs(bLen - tLen) > 0.5f || abs(bThick - tThick) > 0.1f) {
        vboMesh.clear();
        ofSetRandomSeed(seed);
        // 構造体 s を経由して描画パラメータを渡す
        buildBranchMesh(bLen * s.lenScale, bThick * s.thickScale, depthLevel, glm::mat4(1.0), chaosResist, bloomLevel, gType, fType);
        bNeedsUpdate = false;
    }
}

void Tree::draw() {
    vboMesh.draw();
}

void Tree::water(float buff, int resilienceLevel, float increment) {
    // デメリット軽減係数 (1レベルにつき15%軽減)
    float penaltyFactor = 1.0f - (resilienceLevel * 0.15f);

    depthExp += 5.0f * buff;

    float lenGain = increment * 2.0f * buff;
    tLen += lenGain;
    totalLenEarned += lenGain; // 累積加算

    // デメリット（太さ減少）に軽減を適用
    tThick = max(2.0f, tThick - (3.0f * penaltyFactor));
    tMutation = max(0.0f, tMutation - 0.1f);
}

void Tree::fertilize(float buff, int resilienceLevel, float increment) {
    float penaltyFactor = 1.0f - (resilienceLevel * 0.15f);

    depthExp += 5.0f * buff;

    // デメリット（長さ減少）に軽減を適用
    tLen = max(10.0f, tLen - (5.0f * penaltyFactor));

    float thickGain = increment * 2.0f * buff;
    tThick += thickGain;
    totalThickEarned += thickGain; // 累積加算

    tMutation = max(0.0f, tMutation - 0.05f);
}

void Tree::kotodama(float buff) {
    depthExp += 5.0f * buff;
    tLen += 10.0f * buff;
    tThick = max(2.0f, tThick - 5.0f);

    float mutGain = 0.2f * buff;
    tMutation = ofClamp(tMutation + mutGain, 0.0f, 1.0f);
    totalMutationEarned += mutGain; // 累積加算
}

void Tree::applyEvolution(GrowthType type) {
    switch (type) {
    case TYPE_ELEGANT:
        s.branchLenRatio = 0.82f;   // より長く伸びる
        s.baseAngle = 18.0f;        // 鋭角でスマートな印象
        s.twistFactor = 15.0f;      // 控えめなねじれ
        break;
    case TYPE_STURDY:
        s.branchThickRatio = 0.85f; // 太さを維持
        s.baseAngle = 35.0f;        // どっしりと広がる
        s.twistFactor = 40.0f;      // 力強いねじれ
        break;
    case TYPE_ELDRITCH:
        s.mutationAngleMax = 65.0f; // 予測不能な広がり
        s.twistFactor = 150.0f;     // 激しい螺旋
        s.trunkHueEnd += 100.0f;    // 色彩変化を拡大
        break;
    }
    bNeedsUpdate = true; // メッシュを再構築
}

glm::mat4 Tree::getNextBranchMatrix(glm::mat4 tipMat, int index, int total, float angleBase) {
    glm::mat4 m = tipMat;
    // Y軸回転で円状に配置
    m = glm::rotate(m, glm::radians(index * (360.0f / total)), glm::vec3(0, 1, 0));
    // 外側へ倒す回転（カオス度による揺らぎ）
    float wobble = ofRandom(-10, 10) * bMutation;
    m = glm::rotate(m, glm::radians(angleBase + wobble), glm::vec3(0, 0, 1));
    return m;
}

void Tree::buildBranchMesh(float length, float thickness, int depth, glm::mat4 mat, int chaosResist, int bloomLevel, GrowthType gType, FlowerType fType) {
    if (depth < 0) return;

    // 現在の枝（幹）をメッシュに追加
    addStemToMesh(thickness, thickness * s.branchThickRatio, length, mat, chaosResist, depth, gType);

    // 枝の先端の行列を計算
    glm::mat4 tipMat = glm::translate(mat, glm::vec3(0, length, 0));

    // --- 装飾（葉・花）のロジック ---
    float bloomThreshold = 0.4f - (bloomLevel * 0.05f);
    bool isBloomed = (maxMutationReached > bloomThreshold);

    if (depth == 0 && (isBloomed || fType != FLOWER_NONE)) {
        addFlowerToMesh(thickness, tipMat, fType);
    }
    else if (depth <= 1) {
        addLeafToMesh(thickness, tipMat);
    }

    // --- 次の枝への再帰 ---
    int numBranches = (depth < 2) ? 2 : 3;
    float angleBase = 25.0f + (bMutation * 45.0f); // カオス度で分岐角が広がる

    for (int i = 0; i < numBranches; i++) {
        glm::mat4 childMat = getNextBranchMatrix(tipMat, i, numBranches, angleBase);
        buildBranchMesh(length * s.branchLenRatio, thickness * s.branchThickRatio, depth - 1, childMat, chaosResist, bloomLevel, gType, fType);
    }
}

void Tree::addStemToMesh(float r1, float r2, float h, glm::mat4 mat, int chaosResist, int depth, GrowthType gType) {
    int segments = (depth <= 4) ? 3 : 5; // LOD: 深い枝ほど角数を減らす
    int subdivisions = 4;                // 縦方向の分割数
    int numRings = subdivisions + 1;

    // --- 色の計算 ---
    float timeShift = ofGetElapsedTimef() * 20.0f;
    if (gType == TYPE_ELDRITCH) {
        timeShift = ofGetElapsedTimef() * 100.0f; // Eldritchは激しく色が動く
    }
    float hueBase = ofMap(bMutation, 0, 1, s.trunkHueStart, s.trunkHueEnd);
    float finalHue = fmod(hueBase + timeShift + (depth * 10), 255.0f);
    ofColor col = ofColor::fromHsb(finalHue, 160, 180 + (depth * 10));
    
    float collapseThreshold = 0.9f + (chaosResist * 0.02f);
    // 法線変換用の行列
    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(mat));

    // 現在のVBOの頂点開始インデックスを記録
    int startIndex = vboMesh.getNumVertices();

    // 1. 頂点と法線の生成
    for (int ring = 0; ring < numRings; ring++) {
        float ratio = (float)ring / subdivisions;
        float currentR = ofLerp(r1, r2, ratio); // テーパリング
        float currentY = h * ratio;

        // 進化タイプや設定に応じた「ねじれ」の適用
        float twistAngle = glm::radians(s.twistFactor * ratio);

        for (int i = 0; i < segments; i++) {
            float angle = (i * TWO_PI / segments) + twistAngle;
            glm::vec3 unitPos(cos(angle), 0, sin(angle));

            // 頂点座標（ローカル）
            glm::vec4 p(unitPos.x * currentR, currentY, unitPos.z * currentR, 1);

            // カオス度が高い場合の頂点ノイズ（最上段に近いほど強く揺らす）
            if (maxMutationReached > 0.5f && ring > 0) {
                float nStr = ofMap(maxMutationReached, 0.5, 1.0, 0, 120.0f, true) * ratio;
                p.x += ofSignedNoise(p.x * 0.1, p.y * 0.1, ofGetElapsedTimef()) * nStr;
                p.z += ofSignedNoise(p.z * 0.1, p.y * 0.1, ofGetElapsedTimef() + 10) * nStr;
            }

            // VBOへの登録
            vboMesh.addVertex(glm::vec3(mat * p));
            vboMesh.addNormal(normalMatrix * unitPos); // 簡易法線
            vboMesh.addColor(col);
        }
    }

    // 2. インデックスの生成（面を貼る）
    for (int ring = 0; ring < subdivisions; ring++) {
        for (int i = 0; i < segments; i++) {
            int nextI = (i + 1) % segments;

            // 現在の層の2点
            int v0 = startIndex + (ring * segments) + i;
            int v1 = startIndex + (ring * segments) + nextI;
            // 次の層の2点
            int v2 = startIndex + ((ring + 1) * segments) + i;
            int v3 = startIndex + ((ring + 1) * segments) + nextI;

            // 三角形1
            vboMesh.addIndex(v0);
            vboMesh.addIndex(v1);
            vboMesh.addIndex(v2);

            // 三角形2
            vboMesh.addIndex(v1);
            vboMesh.addIndex(v3);
            vboMesh.addIndex(v2);
        }
    }
}

float Tree::getExpForDepth(int d) {
    if (d <= 0) return 0;
    return s.expBase * pow((float)d, s.expPower);
}

float Tree::getDepthProgress() {
    // 現在のレベルと次のレベルに必要な経験値を取得
    float curThreshold = getExpForDepth(depthLevel);
    float nxtThreshold = getExpForDepth(depthLevel + 1);

    // 0除算を防ぐ
    if (nxtThreshold <= curThreshold) return 1.0f;

    // 現在のレベル内での進捗率を 0.0 ~ 1.0 で返す
    return ofClamp((depthExp - curThreshold) / (nxtThreshold - curThreshold), 0.0f, 1.0f);
}

void Tree::addLeafToMesh(float thickness, glm::mat4 mat) {
    int startIndex = vboMesh.getNumVertices();
    ofColor lCol = s.leafColor;
    float w = thickness * 3.0f;
    float h = thickness * 6.0f;

    // 4頂点 (ひし形)
    vboMesh.addVertex(glm::vec3(mat * glm::vec4(0, 0, 0, 1)));           // 0: 付け根
    vboMesh.addVertex(glm::vec3(mat * glm::vec4(-w, h * 0.5f, 0, 1)));   // 1: 左
    vboMesh.addVertex(glm::vec3(mat * glm::vec4(w, h * 0.5f, 0, 1)));    // 2: 右
    vboMesh.addVertex(glm::vec3(mat * glm::vec4(0, h, 0, 1)));           // 3: 先端

    for (int i = 0; i < 4; i++) {
        vboMesh.addNormal(glm::normalize(glm::mat3(mat) * glm::vec3(0, 0, 1)));
        vboMesh.addColor(lCol);
    }

    // インデックスで2つの三角形を形成
    vboMesh.addIndex(startIndex + 0); vboMesh.addIndex(startIndex + 1); vboMesh.addIndex(startIndex + 3);
    vboMesh.addIndex(startIndex + 0); vboMesh.addIndex(startIndex + 2); vboMesh.addIndex(startIndex + 3);
}

void Tree::addFlowerToMesh(float thickness, glm::mat4 mat, FlowerType type) {
    if (type == FLOWER_NONE) return;

    int startIndex = vboMesh.getNumVertices();
    ofColor fCol = s.flowerColor;

    if (type == FLOWER_CRYSTAL) {
        // 【Type A: 結晶】 放射状に広がる鋭い三角形
        float r = thickness * 4.0f;
        int numPoints = 6;
        vboMesh.addVertex(glm::vec3(mat * glm::vec4(0, 0, 0, 1))); // 中心
        vboMesh.addColor(fCol); vboMesh.addNormal(glm::vec3(0, 1, 0));

        for (int i = 0; i < numPoints; i++) {
            float ang = i * TWO_PI / numPoints;
            vboMesh.addVertex(glm::vec3(mat * glm::vec4(cos(ang) * r, thickness, sin(ang) * r, 1)));
            vboMesh.addColor(fCol); vboMesh.addNormal(glm::vec3(0, 1, 0));

            vboMesh.addIndex(startIndex);
            vboMesh.addIndex(startIndex + 1 + i);
            vboMesh.addIndex(startIndex + 1 + (i + 1) % numPoints);
        }
    }
    else if (type == FLOWER_PETAL) {
        // 【Type B: 花弁】 5枚の柔らかい面
        float r = thickness * 3.5f;
        for (int i = 0; i < 5; i++) {
            int pStart = vboMesh.getNumVertices();
            float ang = i * TWO_PI / 5;
            // 簡易的な花びら1枚(三角形)
            vboMesh.addVertex(glm::vec3(mat * glm::vec4(0, 0, 0, 1)));
            vboMesh.addVertex(glm::vec3(mat * glm::vec4(cos(ang - 0.3) * r, r * 0.5, sin(ang - 0.3) * r, 1)));
            vboMesh.addVertex(glm::vec3(mat * glm::vec4(cos(ang + 0.3) * r, r * 0.5, sin(ang + 0.3) * r, 1)));
            for (int k = 0; k < 3; k++) { vboMesh.addColor(fCol); vboMesh.addNormal(glm::vec3(0, 1, 0)); }
            vboMesh.addIndex(pStart); vboMesh.addIndex(pStart + 1); vboMesh.addIndex(pStart + 2);
        }
    }
    else if (type == FLOWER_SPIRIT) {
        // 【Type C: 霊魂】 ゆらゆら揺れる尖った火の玉
        float r = thickness * 2.5f;
        float time = ofGetElapsedTimef() * 3.0f;
        float offset = ofSignedNoise(time) * 15.0f;

        vboMesh.addVertex(glm::vec3(mat * glm::vec4(offset, r * 5.0f, 0, 1))); // 尖った先端
        vboMesh.addVertex(glm::vec3(mat * glm::vec4(-r, 0, -r, 1)));
        vboMesh.addVertex(glm::vec3(mat * glm::vec4(r, 0, -r, 1)));
        vboMesh.addVertex(glm::vec3(mat * glm::vec4(0, 0, r, 1)));

        for (int k = 0; k < 4; k++) { vboMesh.addColor(ofColor(150, 200, 255, 180)); vboMesh.addNormal(glm::vec3(0, 1, 0)); }
        // 四面体のインデックス
        int idxs[] = { 0,1,2, 0,2,3, 0,3,1 };
        for (int id : idxs) vboMesh.addIndex(startIndex + id);
    }
}

void Tree::addJointToMesh(float radius, glm::mat4 mat, ofColor col, int depth) {
    // LOD: 先端の細い枝ほどポリゴンを削る
    int rings = (depth <= 2) ? 4 : 6;
    int sectors = (depth <= 2) ? 4 : 6;
    int startIndex = vboMesh.getNumVertices();
    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(mat));

    for (int r = 0; r <= rings; r++) {
        float phi = PI * (float)r / rings;
        for (int s = 0; s <= sectors; s++) {
            float theta = TWO_PI * (float)s / sectors;

            glm::vec3 unitPos(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
            vboMesh.addVertex(glm::vec3(mat * glm::vec4(unitPos * radius, 1.0)));
            vboMesh.addNormal(normalMatrix * unitPos);
            vboMesh.addColor(col);
        }
    }

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            int v0 = startIndex + r * (sectors + 1) + s;
            int v1 = v0 + 1;
            int v2 = startIndex + (r + 1) * (sectors + 1) + s;
            int v3 = v2 + 1;
            vboMesh.addIndex(v0); vboMesh.addIndex(v1); vboMesh.addIndex(v2);
            vboMesh.addIndex(v1); vboMesh.addIndex(v3); vboMesh.addIndex(v2);
        }
    }
}

void Tree::reset() {
    // 育成状態の初期化
    dayCount = 1;
    maxMutationReached = 0;
    depthExp = 0;
    depthLevel = 0;
    // パラメータを初期値へ
    bLen = 0; tLen = 10;
    bThick = 0; tThick = 2;
    bMutation = 0; tMutation = 0;

    // 木の形状シードを再生成
    seed = ofRandom(99999);
    vboMesh.clear();
    bNeedsUpdate = true;
}

int Tree::getCurrentDepth() {
    return depthLevel;
}