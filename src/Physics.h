#pragma once

class Ball;
class Food;
class Thorn;
class Spore;

class Physics {
public:
    // Ball ³Ô Food
    static bool canEat(const Ball& eater, const Food& food);
    // Ball ³Ô Spore
    static bool canEat(const Ball& eater, const Spore& spore);
    // Ball ³Ô Thorn
    static bool canEat(const Ball& eater, const Thorn& thorn, float ratio = 1.0f);
    // Ball ³Ô Ball
    static bool canEat(const Ball& eater, const Ball& other, float ratio = 1.0f);
};
