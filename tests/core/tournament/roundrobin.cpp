#include "core/tournament/roundrobin.hpp"
#include <doctest/doctest.h>

TEST_SUITE("Tournament - Round Robin") {
    TEST_CASE("Test 1") {
        const auto num_players = 2;
        const auto num_games = 2;
        const auto num_openings = 2;
        auto gen = RoundRobinGenerator(num_players, num_games, num_openings, true);

        REQUIRE(gen.expected() == 2);

        // id, opening, player1, player2
        REQUIRE(gen.next() == GameInfo{0, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{1, 0, 1, 0});

        // Overflow
        REQUIRE(gen.next() == GameInfo{2, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{3, 0, 1, 0});

        // Extra overflow
        REQUIRE(gen.next() == GameInfo{4, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{5, 0, 1, 0});
        REQUIRE(gen.next() == GameInfo{6, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{7, 0, 1, 0});
        REQUIRE(gen.next() == GameInfo{8, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{9, 0, 1, 0});
    }

    TEST_CASE("Test 2") {
        const auto num_players = 2;
        const auto num_games = 4;
        const auto num_openings = 2;
        auto gen = RoundRobinGenerator(num_players, num_games, num_openings, true);

        REQUIRE(gen.expected() == 4);

        // id, opening, player1, player2
        REQUIRE(gen.next() == GameInfo{0, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{1, 0, 1, 0});
        REQUIRE(gen.next() == GameInfo{2, 1, 0, 1});
        REQUIRE(gen.next() == GameInfo{3, 1, 1, 0});

        // Overflow
        REQUIRE(gen.next() == GameInfo{4, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{5, 0, 1, 0});
        REQUIRE(gen.next() == GameInfo{6, 1, 0, 1});
        REQUIRE(gen.next() == GameInfo{7, 1, 1, 0});
    }

    TEST_CASE("Test 3") {
        const auto num_players = 4;
        const auto num_games = 2;
        const auto num_openings = 2;
        auto gen = RoundRobinGenerator(num_players, num_games, num_openings, true);

        REQUIRE(gen.expected() == 12);

        // id, opening, player1, player2
        REQUIRE(gen.next() == GameInfo{0, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{1, 0, 1, 0});
        REQUIRE(gen.next() == GameInfo{2, 0, 0, 2});
        REQUIRE(gen.next() == GameInfo{3, 0, 2, 0});
        REQUIRE(gen.next() == GameInfo{4, 0, 0, 3});
        REQUIRE(gen.next() == GameInfo{5, 0, 3, 0});

        REQUIRE(gen.next() == GameInfo{6, 0, 1, 2});
        REQUIRE(gen.next() == GameInfo{7, 0, 2, 1});
        REQUIRE(gen.next() == GameInfo{8, 0, 1, 3});
        REQUIRE(gen.next() == GameInfo{9, 0, 3, 1});

        REQUIRE(gen.next() == GameInfo{10, 0, 2, 3});
        REQUIRE(gen.next() == GameInfo{11, 0, 3, 2});

        // Overflow
        REQUIRE(gen.next() == GameInfo{12, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{13, 0, 1, 0});
    }

    TEST_CASE("Test no repeat") {
        const auto num_players = 2;
        const auto num_games = 2;
        const auto num_openings = 2;
        auto gen = RoundRobinGenerator(num_players, num_games, num_openings, false);

        REQUIRE(gen.expected() == 2);

        // id, opening, player1, player2
        REQUIRE(gen.next() == GameInfo{0, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{1, 1, 0, 1});

        // Overflow
        REQUIRE(gen.next() == GameInfo{2, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{3, 1, 0, 1});

        // Extra overflow
        REQUIRE(gen.next() == GameInfo{4, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{5, 1, 0, 1});
        REQUIRE(gen.next() == GameInfo{6, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{7, 1, 0, 1});
        REQUIRE(gen.next() == GameInfo{8, 0, 0, 1});
        REQUIRE(gen.next() == GameInfo{9, 1, 0, 1});
    }
}
