#include "Tree.h"
void Tree::setup() {
    seed = ofRandom(99999);
    maxMutationReached = 0;
}

void Tree::update() {
    // 全てのパラメータを滑らかに補間
    bLen = ofLerp(bLen, tLen, 0.1f);
    bThick = ofLerp(bThick, tThick, 0.1f);
    bMutation = ofLerp(bMutation, tMutation, 0.1f);
    currentDepth = ofClamp((int)(bLen / 40.0f), 0, MAX_DEPTH);
    // 過去最大のカオス度を記録
    if (bMutation > maxMutationReached) {
        maxMutationReached = bMutation;
    }
    // VBOの更新
    vboMesh.clear();
    vboMesh.setMode(OF_PRIMITIVE_TRIANGLES);
    ofSetRandomSeed(seed);
    buildBranchMesh(bLen, bThick, currentDepth, glm::mat4(1.0));
}

void Tree::draw() {
    vboMesh.draw();
}

void Tree::water(float buff) {
    tLen += 15.0f * buff;
    tMutation = max(0.0f, tMutation - 0.05f); // 水・肥料で減少
}

void Tree::fertilize(float buff) {
    tThick += 2.0f * buff;
    tMutation = max(0.0f, tMutation - 0.05f); // 水・肥料で減少
}

void Tree::kotodama(float buff) {
    tMutation = ofClamp(tMutation + 0.1f * buff, 0.0f, 1.0f); // 言霊で上昇
}

void Tree::buildBranchMesh(float length, float thickness, int depth, glm::mat4 mat) {
    if (depth < 0) return;

    // 1. 現在の枝を描画
    addStemToMesh(thickness, thickness * 0.7f, length, mat);

    // 2. 枝の先端の位置行列を取得（装飾の配置用）
    glm::mat4 tipMat = glm::translate(mat, glm::vec3(0, length, 0));

    // 3. 装飾ロジックの適用
    bool isBloomed = (maxMutationReached > 0.4);

    if (isBloomed) {
        if (depth == 0) {
            addFlowerToMesh(thickness, tipMat); // 先端に球体の花
        }
        else if (depth == 1 || depth == 2) {
            addLeafToMesh(thickness, tipMat);   // 1,2段目に葉
        }
    }
    else {
        if (depth == 0 || depth == 1) {
            addLeafToMesh(thickness, tipMat);   // 未開花時は先端1段目まで葉
        }
    }

    // 4. 次の枝へ再帰
    int numBranches = (depth < 2) ? 2 : 3;
    float angleBase = 25.0f + (bMutation * 45.0f);

    for (int i = 0; i < numBranches; i++) {
        glm::mat4 childMat = tipMat;
        childMat = glm::rotate(childMat, glm::radians(i * (360.0f / numBranches)), glm::vec3(0, 1, 0));
        childMat = glm::rotate(childMat, glm::radians(angleBase + ofRandom(-10, 10) * bMutation), glm::vec3(0, 0, 1));
        buildBranchMesh(length * 0.75f, thickness * 0.7f, depth - 1, childMat);
    }
}

void Tree::addStemToMesh(float r1, float r2, float h, glm::mat4 mat) {
    int segments = 5;
    float hue = ofMap(bMutation, 0, 1, 30, 200) + ofRandom(-20, 20) * bMutation;
    ofColor col = ofColor::fromHsb(hue, 150, 100 + (h * 0.5));

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
        if (bMutation > 0.9f) {
            float time = ofGetElapsedTimef();
            float nStr = (bMutation - 0.9f) * 100.0f;
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
    maxMutationReached = 0; // 開花状態をリセット

    // パラメータを初期値へ
    bLen = 0; tLen = 10;
    bThick = 0; tThick = 2;
    bMutation = 0; tMutation = 0;

    // 木の形状シードを再生成
    seed = ofRandom(99999);
}