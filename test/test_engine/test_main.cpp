#include <unity.h>
#include "Board.h"
#include "MoveGen.h"
#include "Game.h"
#include "ComputerPlayer.h"

void setUp(void) {}
void tearDown(void) {}

void test_initial_setup(void) {
    Board b;
    b.resetStartingPosition();

    TEST_ASSERT_EQUAL(2, b.checkersOf(Player::P0, 24));
    TEST_ASSERT_EQUAL(5, b.checkersOf(Player::P0, 13));
    TEST_ASSERT_EQUAL(3, b.checkersOf(Player::P0, 8));
    TEST_ASSERT_EQUAL(5, b.checkersOf(Player::P0, 6));

    TEST_ASSERT_EQUAL(2, b.checkersOf(Player::P1, 1));
    TEST_ASSERT_EQUAL(5, b.checkersOf(Player::P1, 12));
    TEST_ASSERT_EQUAL(3, b.checkersOf(Player::P1, 17));
    TEST_ASSERT_EQUAL(5, b.checkersOf(Player::P1, 19));

    int totalP0 = 0, totalP1 = 0;
    for (int pt = 1; pt <= 24; ++pt) {
        totalP0 += b.checkersOf(Player::P0, pt);
        totalP1 += b.checkersOf(Player::P1, pt);
    }
    TEST_ASSERT_EQUAL(15, totalP0);
    TEST_ASSERT_EQUAL(15, totalP1);
}

void test_basic_move_and_blocking(void) {
    Board b;
    b.resetStartingPosition();
    std::vector<int> dice = {6, 5};

    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    bool found = false;
    for (auto& m : moves) {
        if (m.from == 24 && m.to == 18 && m.die == 6) found = true;
    }
    TEST_ASSERT_TRUE(found);

    // Block point 18 with two P1 checkers.
    b.points[17] = -2;
    auto moves2 = MoveGen::legalMoves(b, Player::P0, dice);
    for (auto& m : moves2) {
        TEST_ASSERT_FALSE(m.from == 24 && m.to == 18 && m.die == 6);
    }
}

void test_hit_blot(void) {
    Board b;
    b.resetStartingPosition();
    b.points[17] = -1; // single P1 blot on point 18

    Move m{24, 18, 6};
    b.applyMove(Player::P0, m);

    TEST_ASSERT_EQUAL(1, b.checkersOf(Player::P0, 18));
    TEST_ASSERT_EQUAL(0, b.checkersOf(Player::P1, 18));
    TEST_ASSERT_EQUAL(1, b.bar[static_cast<int>(Player::P1)]);
}

void test_bar_entry_required_and_blocked(void) {
    Board b;
    b.resetStartingPosition();
    b.bar[static_cast<int>(Player::P0)] = 1;
    std::vector<int> dice = {3, 5};

    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    TEST_ASSERT_TRUE(moves.size() > 0);
    for (auto& m : moves) TEST_ASSERT_EQUAL(BAR, m.from);

    bool has3 = false, has5 = false;
    for (auto& m : moves) {
        if (m.die == 3) { TEST_ASSERT_EQUAL(22, m.to); has3 = true; }
        if (m.die == 5) { TEST_ASSERT_EQUAL(20, m.to); has5 = true; }
    }
    TEST_ASSERT_TRUE(has3);
    TEST_ASSERT_TRUE(has5);

    // Block the die-3 entry point (22) with two P1 checkers.
    b.points[21] = -2;
    auto moves2 = MoveGen::legalMoves(b, Player::P0, dice);
    for (auto& m : moves2) TEST_ASSERT_FALSE(m.die == 3);
}

void test_two_on_bar_enter_sequentially(void) {
    Board b;
    b.bar[static_cast<int>(Player::P0)] = 2;
    std::vector<int> dice = {3, 5};

    TEST_ASSERT_EQUAL(2, MoveGen::maxPlyLength(b, Player::P0, dice));
    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    TEST_ASSERT_EQUAL(2, moves.size());
    for (auto& m : moves) TEST_ASSERT_EQUAL(BAR, m.from);

    // Enter one with die 3; the second bar checker must still enter with the
    // remaining die (5), not have some other checker move instead.
    b.applyMove(Player::P0, Move{BAR, 22, 3});
    TEST_ASSERT_EQUAL(1, b.bar[static_cast<int>(Player::P0)]);

    std::vector<int> remaining = {5};
    auto moves2 = MoveGen::legalMoves(b, Player::P0, remaining);
    TEST_ASSERT_EQUAL(1, moves2.size());
    TEST_ASSERT_EQUAL(BAR, moves2[0].from);
    TEST_ASSERT_EQUAL(20, moves2[0].to);
}

void test_two_on_bar_partial_entry_when_one_blocked(void) {
    Board b;
    b.bar[static_cast<int>(Player::P0)] = 2;
    b.points[21] = -2; // point 22 (die-3 entry) blocked
    std::vector<int> dice = {3, 5};

    TEST_ASSERT_EQUAL(1, MoveGen::maxPlyLength(b, Player::P0, dice));
    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    TEST_ASSERT_EQUAL(1, moves.size());
    TEST_ASSERT_EQUAL(BAR, moves[0].from);
    TEST_ASSERT_EQUAL(5, moves[0].die);
    TEST_ASSERT_EQUAL(20, moves[0].to);

    // The stranded second checker stays on the bar; nothing else may move.
    b.applyMove(Player::P0, moves[0]);
    TEST_ASSERT_EQUAL(1, b.bar[static_cast<int>(Player::P0)]);
}

void test_two_on_bar_with_doubles(void) {
    Board b;
    b.bar[static_cast<int>(Player::P0)] = 2;
    std::vector<int> dice = {4, 4, 4, 4};

    TEST_ASSERT_EQUAL(4, MoveGen::maxPlyLength(b, Player::P0, dice));

    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    TEST_ASSERT_EQUAL(1, moves.size());
    TEST_ASSERT_EQUAL(BAR, moves[0].from);
    TEST_ASSERT_EQUAL(21, moves[0].to); // 25 - 4

    b.applyMove(Player::P0, moves[0]);
    TEST_ASSERT_EQUAL(1, b.bar[static_cast<int>(Player::P0)]);

    std::vector<int> remaining = {4, 4, 4};
    auto moves2 = MoveGen::legalMoves(b, Player::P0, remaining);
    TEST_ASSERT_EQUAL(1, moves2.size());
    TEST_ASSERT_EQUAL(BAR, moves2[0].from);
    TEST_ASSERT_EQUAL(21, moves2[0].to);

    b.applyMove(Player::P0, moves2[0]);
    TEST_ASSERT_EQUAL(0, b.bar[static_cast<int>(Player::P0)]);
    TEST_ASSERT_EQUAL(2, b.checkersOf(Player::P0, 21));

    // Bar now empty — remaining dice move normally from point 21.
    std::vector<int> lastTwo = {4, 4};
    auto moves3 = MoveGen::legalMoves(b, Player::P0, lastTwo);
    bool foundNormalMove = false;
    for (auto& m : moves3) {
        if (m.from == 21) foundNormalMove = true;
        TEST_ASSERT_FALSE(m.from == BAR);
    }
    TEST_ASSERT_TRUE(foundNormalMove);
}

void test_bear_off_exact(void) {
    Board b;
    b.points[0] = 2; // point 1
    b.points[1] = 2; // point 2
    b.points[2] = 2; // point 3
    b.points[3] = 2; // point 4
    b.points[4] = 3; // point 5
    b.points[5] = 4; // point 6
    TEST_ASSERT_TRUE(b.allCheckersHome(Player::P0));

    std::vector<int> dice = {6, 6, 6, 6};
    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    bool found = false;
    for (auto& m : moves) {
        if (m.from == 6 && m.to == OFF && m.die == 6) found = true;
    }
    TEST_ASSERT_TRUE(found);
}

void test_bear_off_overshoot_rules(void) {
    Board b;
    b.points[4] = 1; // point 5, sole remaining checker
    b.off[0] = 14;
    std::vector<int> dice = {6};

    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    bool overshootLegal = false;
    for (auto& m : moves) {
        if (m.from == 5 && m.to == OFF && m.die == 6) overshootLegal = true;
    }
    TEST_ASSERT_TRUE(overshootLegal);

    // Add a checker further from home (point 6) — now the point-5 overshoot
    // is illegal, but the exact bear-off from point 6 is legal.
    Board b2 = b;
    b2.points[5] = 1;
    b2.off[0] = 13;
    auto moves2 = MoveGen::legalMoves(b2, Player::P0, dice);

    bool overshootStillLegal = false;
    bool exactFromSix = false;
    for (auto& m : moves2) {
        if (m.from == 5 && m.to == OFF) overshootStillLegal = true;
        if (m.from == 6 && m.to == OFF) exactFromSix = true;
    }
    TEST_ASSERT_FALSE(overshootStillLegal);
    TEST_ASSERT_TRUE(exactFromSix);
}

// Mirrors test_bear_off_exact but for P1, whose bear-off direction/home
// board (19-24) is the opposite of P0's — the exact scenario reported from
// real play: P1 with its last 4 checkers stacked on point 24 (P1's own
// "1-point"), rolling double 1s, should bear off all four.
void test_bear_off_exact_p1(void) {
    Board b;
    b.points[23] = -4; // point 24, P1's last 4 checkers (negative = P1)
    b.off[1] = 11;
    TEST_ASSERT_TRUE(b.allCheckersHome(Player::P1));

    std::vector<int> dice = {1, 1, 1, 1};
    TEST_ASSERT_EQUAL(4, MoveGen::maxPlyLength(b, Player::P1, dice));

    std::vector<int> remaining = dice;
    for (int i = 0; i < 4; ++i) {
        auto moves = MoveGen::legalMoves(b, Player::P1, remaining);
        bool found = false;
        for (auto& m : moves) {
            if (m.from == 24 && m.to == OFF && m.die == 1) found = true;
        }
        TEST_ASSERT_TRUE(found);
        b.applyMove(Player::P1, Move{24, OFF, 1});
        remaining.erase(remaining.begin());
    }

    TEST_ASSERT_EQUAL(15, b.off[1]);
    TEST_ASSERT_TRUE(b.isGameOver());
    TEST_ASSERT_EQUAL(static_cast<int>(Player::P1), static_cast<int>(b.winner()));
}

void test_doubles_give_four_dice(void) {
    Roll r{4, 4};
    auto dice = r.toDice();
    TEST_ASSERT_EQUAL(4, dice.size());
    for (int d : dice) TEST_ASSERT_EQUAL(4, d);
}

void test_forced_larger_die_when_only_one_playable(void) {
    Board b;
    b.points[9] = 1;  // point 10: sole P0 checker
    b.points[2] = -2; // point 3: blocked by P1 (both dice would land here)
    std::vector<int> dice = {2, 5};

    auto moves = MoveGen::legalMoves(b, Player::P0, dice);
    TEST_ASSERT_EQUAL(1, moves.size());
    TEST_ASSERT_EQUAL(10, moves[0].from);
    TEST_ASSERT_EQUAL(5, moves[0].to);
    TEST_ASSERT_EQUAL(5, moves[0].die);
}

void test_game_turn_flow(void) {
    std::vector<int> scripted = {6, 5};
    size_t idx = 0;
    Game game([&scripted, &idx]() { return scripted[idx++ % scripted.size()]; });

    TEST_ASSERT_EQUAL(static_cast<int>(GamePhase::WaitingToRoll), static_cast<int>(game.phase()));

    game.roll();
    TEST_ASSERT_EQUAL(static_cast<int>(GamePhase::MovingCheckers), static_cast<int>(game.phase()));
    TEST_ASSERT_EQUAL(2, game.diceRemaining().size());

    auto legal = game.currentLegalMoves();
    TEST_ASSERT_TRUE(legal.size() > 0);
    TEST_ASSERT_TRUE(game.playMove(legal[0]));
    TEST_ASSERT_EQUAL(1, game.diceRemaining().size());

    auto legal2 = game.currentLegalMoves();
    TEST_ASSERT_TRUE(legal2.size() > 0);
    TEST_ASSERT_TRUE(game.playMove(legal2[0]));

    TEST_ASSERT_EQUAL(static_cast<int>(Player::P1), static_cast<int>(game.toMove()));
    TEST_ASSERT_EQUAL(static_cast<int>(GamePhase::WaitingToRoll), static_cast<int>(game.phase()));
}

void test_computer_prefers_hit(void) {
    Board b;
    b.points[4] = 1; // point 5: P0 blot (opponent, from P1's perspective)

    Move hitMove{10, 5, 5};  // lands on point 5 -> hits the blot
    Move safeMove{10, 8, 2}; // lands on empty point 8 -> no hit
    std::vector<Move> options = {safeMove, hitMove};

    Move chosen = ComputerPlayer::chooseMove(b, Player::P1, options);
    TEST_ASSERT_EQUAL(5, chosen.to);
}

void test_computer_prefers_making_point_over_blot(void) {
    Board b;
    b.points[7] = -1; // point 8: P1 already has a single checker there

    Move makePointMove{10, 8, 2}; // joins the existing checker -> makes a point
    Move blotMove{10, 14, 4};     // lands on an empty point -> leaves a lone blot
    std::vector<Move> options = {blotMove, makePointMove};

    Move chosen = ComputerPlayer::chooseMove(b, Player::P1, options);
    TEST_ASSERT_EQUAL(8, chosen.to);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_setup);
    RUN_TEST(test_basic_move_and_blocking);
    RUN_TEST(test_hit_blot);
    RUN_TEST(test_bar_entry_required_and_blocked);
    RUN_TEST(test_two_on_bar_enter_sequentially);
    RUN_TEST(test_two_on_bar_partial_entry_when_one_blocked);
    RUN_TEST(test_two_on_bar_with_doubles);
    RUN_TEST(test_bear_off_exact);
    RUN_TEST(test_bear_off_overshoot_rules);
    RUN_TEST(test_bear_off_exact_p1);
    RUN_TEST(test_doubles_give_four_dice);
    RUN_TEST(test_forced_larger_die_when_only_one_playable);
    RUN_TEST(test_game_turn_flow);
    RUN_TEST(test_computer_prefers_hit);
    RUN_TEST(test_computer_prefers_making_point_over_blot);
    return UNITY_END();
}
