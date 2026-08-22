#pragma once
#include "Board.h"
#include "Dice.h"
#include "MoveGen.h"

enum class GamePhase { WaitingToRoll, MovingCheckers, GameOver };

// Orchestrates a single local pass-and-play game: whose turn it is, the
// dice remaining to play this turn, and applying/validating moves. Pure
// game-state logic, no I/O — the UI layer drives it.
class Game {
public:
    explicit Game(DieRoller roller);

    void reset();
    void roll(); // valid only when phase() == WaitingToRoll
    bool playMove(const Move& m); // false if illegal; advances turn automatically

    std::vector<Move> currentLegalMoves() const;

    const Board& board() const { return board_; }
    Player toMove() const { return toMove_; }
    GamePhase phase() const { return phase_; }
    const std::vector<int>& diceRemaining() const { return diceRemaining_; }
    Player winnerPlayer() const { return board_.winner(); }

private:
    Board board_;
    Player toMove_ = Player::P0;
    GamePhase phase_ = GamePhase::WaitingToRoll;
    std::vector<int> diceRemaining_;
    DieRoller roller_;

    void endTurnIfDone();
};
