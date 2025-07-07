#pragma once

class Ball;
class Food;
class Thorn;
class Spore;

class Physics {
public:
    // Ball �� Food
    static bool canEat(const Ball& eater, const Food& food);
    // Ball �� Spore
    static bool canEat(const Ball& eater, const Spore& spore);
    // Ball �� Thorn
    static bool canEat(const Ball& eater, const Thorn& thorn, float ratio = 1.0f);
    // Ball �� Ball
    static bool canEat(const Ball& eater, const Ball& other, float ratio = 1.0f);
};
