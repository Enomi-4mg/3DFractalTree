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

void Tree::update(int growthLevel, int chaosResist, int bloomLevel) {
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
        buildBranchMesh(bLen * s.lenScale, bThick * s.thickScale, depthLevel, glm::mat4(1.0), chaosResist, bloomLevel);
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

glm::mat4 Tree::getNextBranchMatrix(glm::mat4 tipMat, int index, int total, float angleBase) {
    glm::mat4 m = tipMat;
    // Y軸回転で円状に配置
    m = glm::rotate(m, glm::radians(index * (360.0f / total)), glm::vec3(0, 1, 0));
    // 外側へ倒す回転（カオス度による揺らぎ）
    float wobble = ofRandom(-10, 10) * bMutation;
    m = glm::rotate(m, glm::radians(angleBase + wobble), glm::vec3(0, 0, 1));
    return m;
}

void Tree::buildBranchMesh(float length, float thickness, int depth, glm::mat4 mat, int chaosResist, int bloomLevel) {
    if (depth < 0) return;

    // 現在の枝（幹）をメッシュに追加
    addStemToMesh(thickness, thickness * s.branchThickRatio, length, mat, chaosResist, depth);

    // 枝の先端の行列を計算
    glm::mat4 tipMat = glm::translate(mat, glm::vec3(0, length, 0));

    // --- 装飾（葉・花）のロジック ---
    float bloomThreshold = 0.4f - (bloomLevel * 0.05f);
    bool isBloomed = (maxMutationReached > bloomThreshold);

    if (isBloomed) {
        if (depth == 0) addFlowerToMesh(thickness, tipMat);
        else if (depth <= 1) addLeafToMesh(thickness, tipMat);
    }
    else if (depth <= 1) {
        addLeafToMesh(thickness, tipMat);
    }

    // --- 次の枝への再帰 ---
    int numBranches = (depth < 2) ? 2 : 3;
    float angleBase = 25.0f + (bMutation * 45.0f); // カオス度で分岐角が広がる

    for (int i = 0; i < numBranches; i++) {
        glm::mat4 childMat = getNextBranchMatrix(tipMat, i, numBranches, angleBase);
        buildBranchMesh(length * s.branchLenRatio, thickness * s.branchThickRatio, depth - 1, childMat, chaosResist, bloomLevel);
    }
}

void Tree::addStemToMesh(float r1, float r2, float h, glm::mat4 mat, int chaosResist, int depth) {
    int segments = (depth <= 4) ? 3 : 5; // LOD: 深い枝ほど角数を減らす
    int subdivisions = 4;                // 縦方向の分割数
    int numRings = subdivisions + 1;

    // --- 色の計算 ---
    float timeShift = ofGetElapsedTimef() * 20.0f;
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
    ofColor lCol(60, 150, 60, 200);
    float w = thickness * 3.0f;
    float h = thickness * 6.0f;

    // 葉の4頂点（ひし形）
    glm::vec4 p1(0, 0, 0, 1);           // 付け根
    glm::vec4 p2(-w, h * 0.5f, 0, 1);   // 左
    glm::vec4 p3(w, h * 0.5f, 0, 1);    // 右
    glm::vec4 p4(0, h, 0, 1);           // 先端

    glm::vec3 n(0, 0, 1); // 法線

    // 三角形1
    vboMesh.addVertex(glm::vec3(mat * p1)); vboMesh.addNormal(n); vboMesh.addColor(lCol);
    vboMesh.addVertex(glm::vec3(mat * p2)); vboMesh.addNormal(n); vboMesh.addColor(lCol);
    vboMesh.addVertex(glm::vec3(mat * p4)); vboMesh.addNormal(n); vboMesh.addColor(lCol);
    // 三角形2
    vboMesh.addVertex(glm::vec3(mat * p1)); vboMesh.addNormal(n); vboMesh.addColor(lCol);
    vboMesh.addVertex(glm::vec3(mat * p3)); vboMesh.addNormal(n); vboMesh.addColor(lCol);
    vboMesh.addVertex(glm::vec3(mat * p4)); vboMesh.addNormal(n); vboMesh.addColor(lCol);
}

void Tree::addFlowerToMesh(float thickness, glm::mat4 mat) {
    ofColor fCol(255, 180, 200);
    float radius = thickness * 2.2f;
    int res = 6; // 分割数（高くすると重くなります）

    // 球体の頂点生成ロジック
    for (int i = 0; i <= res; i++) {
        float lat0 = PI * (-0.5f + (float)(i - 1) / res);
        float z0 = sin(lat0);
        float zr0 = cos(lat0);

        float lat1 = PI * (-0.5f + (float)i / res);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        for (int j = 0; j <= res; j++) {
            float lng = 2 * PI * (float)(j - 1) / res;
            float x = cos(lng);
            float y = sin(lng);

            // 球体の頂点（法線は中心からのベクトル）
            glm::vec3 n(x * zr1, y * zr1, z1);
            glm::vec3 p = n * radius;

            // 枝の先端（h）の位置に配置されるよう、行列を適用
            vboMesh.addVertex(glm::vec3(mat * glm::vec4(p, 1)));
            vboMesh.addNormal(glm::normalize(glm::mat3(mat) * n));
            vboMesh.addColor(fCol);
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