#include "Dice.h"

Roll rollDice(const DieRoller& roller) {
    Roll r;
    r.a = roller();
    r.b = roller();
    return r;
}
