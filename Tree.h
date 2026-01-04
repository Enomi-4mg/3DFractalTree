#pragma once
#include "ofMain.h"

class Tree {
public:
    void setup();
    void update(float weatherBonus);
    void draw();

    void feed(float amount);
    void mutate(float amount);

private:
    float energy = 0;
    float mutation = 0;
    int currentDepth = 0;
    unsigned int seed = 0;

    void drawBranch(float length, int depth);
}; // © ƒZƒ~ƒRƒƒ“‚ğ–Y‚ê‚¸‚É
