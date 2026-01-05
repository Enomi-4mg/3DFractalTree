#include "Tree.h"
void Tree::setup(const ofJson& config) {
    seed = ofRandom(99999);
    auto treeConf = config.contains("tree") ? config["tree"] : ofJson::object();
    // JSONから定数を取得
    maxDepthConfig = treeConf.value("max_depth", 8);
    depthExpBase = treeConf.value("depth_exp_base", 30.0f);
    depthExpPower = treeConf.value("depth_exp_power", 1.6f);
    lengthVisualScale = treeConf.value("length_visual_scale", 1.5f);
    thickVisualScale = treeConf.value("thickness_visual_scale", 0.8f);
    branchLenRatio = treeConf.value("branch_length_ratio", 0.75f);
    branchThickRatio = treeConf.value("branch_thick_ratio", 0.7f);
    baseAngle = treeConf.value("base_angle", 25.0f);
    mutationAngleMax = treeConf.value("mutation_angle_max", 45.0f);

    auto colorConf = treeConf.contains("colors") ? treeConf["colors"] : ofJson::object();
    trunkHueStart = colorConf.value("trunk_hue_start", 20.0f);
    trunkHueEnd = colorConf.value("trunk_hue_end", 160.0f);

    if (colorConf.contains("leaf") && colorConf["leaf"].is_array()) {
        leafColor = ofColor(colorConf["leaf"][0], colorConf["leaf"][1], colorConf["leaf"][2], colorConf["leaf"][3]);
    }
    else {
        leafColor = ofColor(60, 150, 60, 200);
    }
}

void Tree::update(int growthLevel, int chaosResist, int bloomLevel) {
    // 1. Exp(実際のパラメータ)の補間
    float prevLen = bLen;
    float prevThick = bThick;
    bLen = ofLerp(bLen, tLen, 0.1f);
    bThick = ofLerp(bThick, tThick, 0.1f);
    bMutation = ofLerp(bMutation, tMutation, 0.1f);
    
    if (bMutation > maxMutationReached) {
        maxMutationReached = bMutation;
    }

    // 累進経験値テーブルによる深さ計算
    int newDepth = 0;
    while (newDepth < maxDepthConfig && bThick > getExpForDepth(newDepth + 1)) {
        newDepth++;
    }

    // 数値が動いている、または深さが変わった時にメッシュ更新
    if (newDepth != currentDepth || abs(bLen - prevLen) > 0.01f || abs(bThick - prevThick) > 0.01f) {
        currentDepth = newDepth;
        bNeedsUpdate = true;
    }

    // アニメーション（色彩やノイズ）のために毎フレームフラグを立てることも検討
    if (maxMutationReached > 0.7f) bNeedsUpdate = true;

    if (bNeedsUpdate) {
        vboMesh.clear();
        vboMesh.setMode(OF_PRIMITIVE_TRIANGLES);
        ofSetRandomSeed(seed);
        buildBranchMesh(bLen * lengthVisualScale, bThick * thickVisualScale, currentDepth, glm::mat4(1.0), chaosResist, bloomLevel);
        bNeedsUpdate = false;
    }
}

void Tree::draw() {
    vboMesh.draw();
}

void Tree::water(float buff, int growthLevel, float increment) {
    float bonus = 1.0f + (growthLevel * 0.2f);
    tLen += increment * buff * bonus;
    tMutation = max(0.0f, tMutation - 0.05f); // 水・肥料で減少
}

void Tree::fertilize(float buff, float increment) {
    tThick += increment * buff;
    tMutation = max(0.0f, tMutation - 0.05f); // 水・肥料で減少
}

void Tree::kotodama(float buff) {
    tMutation = ofClamp(tMutation + 0.1f * buff, 0.0f, 1.0f); // 言霊で上昇
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
    addStemToMesh(thickness, thickness * branchThickRatio, length, mat, chaosResist, depth);

    // 枝の先端の行列を計算
    glm::mat4 tipMat = glm::translate(mat, glm::vec3(0, length, 0));

    // --- 装飾（葉・花）のロジック ---
    float bloomThreshold = 0.4f - (bloomLevel * 0.05f);
    bool isBloomed = (maxMutationReached > bloomThreshold);

    if (isBloomed) {
        if (depth == 0) addFlowerToMesh(thickness, tipMat);
        else if (depth <= 2) addLeafToMesh(thickness, tipMat);
    }
    else if (depth <= 1) {
        addLeafToMesh(thickness, tipMat);
    }

    // --- 次の枝への再帰 ---
    int numBranches = (depth < 2) ? 2 : 3;
    float angleBase = 25.0f + (bMutation * 45.0f); // カオス度で分岐角が広がる

    for (int i = 0; i < numBranches; i++) {
        glm::mat4 childMat = getNextBranchMatrix(tipMat, i, numBranches, angleBase);
        buildBranchMesh(length * 0.75f, thickness * 0.7f, depth - 1, childMat, chaosResist, bloomLevel);
    }
}

void Tree::addStemToMesh(float r1, float r2, float h, glm::mat4 mat, int chaosResist, int depth) {
    int segments = (depth <= 4) ? 3 : 5;
    float timeShift = ofGetElapsedTimef() * 20.0f;
    float hueBase = ofMap(bMutation, 0, 1, trunkHueStart, trunkHueEnd);
    float finalHue = fmod(hueBase + timeShift + (depth * 10), 255.0f);
    ofColor col = ofColor::fromHsb(finalHue, 160, 180 + (depth * 10));
    float collapseThreshold = 0.9f + (chaosResist * 0.02f);
    // 法線変換用の行列
    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(mat));

    for (int i = 0; i < segments; i++) {
        float a1 = i * TWO_PI / segments;
        float a2 = (i + 1) * TWO_PI / segments;

        // 側面方向のベクトル（法線の基礎）
        glm::vec3 n1(cos(a1), 0, sin(a1));
        glm::vec3 n2(cos(a2), 0, sin(a2));

        glm::vec4 p1(n1.x * r1, 0, n1.z * r1, 1);
        glm::vec4 p2(n2.x * r1, 0, n2.z * r1, 1);
        glm::vec4 p3(n1.x * r2, h, n1.z * r2, 1);
        glm::vec4 p4(n2.x * r2, h, n2.z * r2, 1);

        // カオス度による形状崩壊 (ofNoise)
        if (maxMutationReached > collapseThreshold) {
            float time = ofGetElapsedTimef();
            float nStr = ofMap(maxMutationReached, 0.5, 1.0, 0, 150.0f, true);
            auto applyNoise = [&](glm::vec4& p) {
                p.x += ofSignedNoise(p.x * 0.1, p.y * 0.1, time) * nStr;
                p.z += ofSignedNoise(p.z * 0.1, p.y * 0.1, time + 10) * nStr;
                };
            applyNoise(p3); applyNoise(p4);
        }

        // 三角形1
        vboMesh.addVertex(glm::vec3(mat * p1)); vboMesh.addNormal(normalMatrix * n1); vboMesh.addColor(col);
        vboMesh.addVertex(glm::vec3(mat * p2)); vboMesh.addNormal(normalMatrix * n2); vboMesh.addColor(col);
        vboMesh.addVertex(glm::vec3(mat * p3)); vboMesh.addNormal(normalMatrix * n1); vboMesh.addColor(col);
        // 三角形2
        vboMesh.addVertex(glm::vec3(mat * p2)); vboMesh.addNormal(normalMatrix * n2); vboMesh.addColor(col);
        vboMesh.addVertex(glm::vec3(mat * p4)); vboMesh.addNormal(normalMatrix * n2); vboMesh.addColor(col);
        vboMesh.addVertex(glm::vec3(mat * p3)); vboMesh.addNormal(normalMatrix * n1); vboMesh.addColor(col);
    }
}

float Tree::getExpForDepth(int d) {
    if (d <= 0) return 0;
    return depthExpBase * pow((float)d, depthExpPower);
}

float Tree::getDepthProgress() {
    float currentThreshold = getExpForDepth(currentDepth);
    float nextThreshold = getExpForDepth(currentDepth + 1);
    if (nextThreshold <= currentThreshold) return 1.0f;
    return ofClamp((bThick - currentThreshold) / (nextThreshold - currentThreshold), 0.0f, 1.0f);
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
    lastMutation = 0;
    // パラメータを初期値へ
    bLen = 0; tLen = 10;
    bThick = 0; tThick = 2;
    bMutation = 0; tMutation = 0;

    // 木の形状シードを再生成
    seed = ofRandom(99999);
    bNeedsUpdate = true;
}