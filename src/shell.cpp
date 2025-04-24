#include "shell.h"

Shell::Shell(Direction direction) : direction_(direction) {
}

Direction& Shell::direction() {
    return direction_;
}

const Direction& Shell::direction() const {
    return direction_;
}

ObjectType Shell::type() const {
    return ObjectType::Shell;
}
