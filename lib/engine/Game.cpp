#include "Game.h"
#include <algorithm>
#include <utility>

Game::Game(DieRoller roller) : roller_(std::move(roller)) {
    reset();
}

void Game::reset() {
    board_ = Board();
    board_.resetStartingPosition();
    toMove_ = Player::P0;
    phase_ = GamePhase::WaitingToRoll;
    diceRemaining_.clear();
}

void Game::roll() {
    if (phase_ != GamePhase::WaitingToRoll) return;

    Roll r = rollDice(roller_);
    diceRemaining_ = r.toDice();
    phase_ = GamePhase::MovingCheckers;

    if (MoveGen::legalMoves(board_, toMove_, diceRemaining_).empty()) {
        // No legal moves at all this turn (danced) — pass automatically.
        diceRemaining_.clear();
        toMove_ = opponent(toMove_);
        phase_ = GamePhase::WaitingToRoll;
    }
}

std::vector<Move> Game::currentLegalMoves() const {
    if (phase_ != GamePhase::MovingCheckers) return {};
    return MoveGen::legalMoves(board_, toMove_, diceRemaining_);
}

bool Game::playMove(const Move& m) {
    if (phase_ != GamePhase::MovingCheckers) return false;

    auto legal = currentLegalMoves();
    bool ok = std::any_of(legal.begin(), legal.end(), [&](const Move& lm) {
        return lm.from == m.from && lm.to == m.to && lm.die == m.die;
    });
    if (!ok) return false;

    board_.applyMove(toMove_, m);
    diceRemaining_.erase(std::find(diceRemaining_.begin(), diceRemaining_.end(), m.die));

    if (board_.isGameOver()) {
        phase_ = GamePhase::GameOver;
        return true;
    }

    endTurnIfDone();
    return true;
}

void Game::endTurnIfDone() {
    if (diceRemaining_.empty() || MoveGen::legalMoves(board_, toMove_, diceRemaining_).empty()) {
        diceRemaining_.clear();
        toMove_ = opponent(toMove_);
        phase_ = GamePhase::WaitingToRoll;
    }
}
