#pragma once

enum class TankAction {
    MoveForward,        // Move the tank forward
    MoveBackward,       // Move the tank backward (with delay)
    RotateLeft_1_8,     // Rotate the tank 1/8 circle (45 degrees) to the left
    RotateRight_1_8,    // Rotate the tank 1/8 circle (45 degrees) to the right
    RotateLeft_1_4,     // Rotate the tank 1/4 circle (90 degrees) to the left
    RotateRight_1_4,    // Rotate the tank 1/4 circle (90 degrees) to the right
    Shoot,              // Shoot a shell in the direction the tank is facing
    Idle			    // Do nothing 
};