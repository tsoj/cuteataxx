#include "play.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include "ataxx/adjudicate.hpp"
#include "ataxx/parse_move.hpp"
#include "engine/engine.hpp"
#include "play.hpp"

[[nodiscard]] constexpr auto make_win_for(const libataxx::Side s) noexcept {
    return s == libataxx::Side::Black ? libataxx::Result::BlackWin : libataxx::Result::WhiteWin;
}

static_assert(make_win_for(libataxx::Side::Black) == libataxx::Result::BlackWin);
static_assert(make_win_for(libataxx::Side::White) == libataxx::Result::WhiteWin);

[[nodiscard]] GameThingy play(const AdjudicationSettings &adjudication,
                              const SearchSettings &tc,
                              const GameSettings &game,
                              std::shared_ptr<Engine> engine1,
                              std::shared_ptr<Engine> engine2) {
    assert(!game.fen.empty());
    assert(game.engine1.id != game.engine2.id);

    GameThingy info;

    // Get engine & position settings
    auto pos = libataxx::Position{game.fen};
    auto game_clock = tc;
    int ply_count = 0;
    info.startpos = pos;

    try {
        engine1->newgame();
        engine2->newgame();

        engine1->isready();
        engine2->isready();

        // Play
        while (!pos.is_gameover()) {
            // Try to adjudicate based on material imbalance
            if (adjudication.material && can_adjudicate_material(pos, *adjudication.material)) {
                info.result = make_win_for(pos.get_turn());
                info.reason = ResultReason::MaterialImbalance;
                break;
            }

            // Try to adjudicate based on "easy fill"
            // This is when one side has to pass and the other can fill the rest of the board trivially to win
            if (adjudication.easyfill && can_adjudicate_easyfill(pos)) {
                info.result = make_win_for(!pos.get_turn());
                info.reason = ResultReason::EasyFill;
                break;
            }

            // Try to adjudicate based on game length
            if (adjudication.gamelength && can_adjudicate_gamelength(pos, *adjudication.gamelength)) {
                info.result = libataxx::Result::Draw;
                info.reason = ResultReason::Gamelength;
                break;
            }

            auto &engine = pos.get_turn() == libataxx::Side::Black ? engine1 : engine2;

            engine->position(pos);

            engine->isready();

            // Start move timer
            const auto t0 = std::chrono::high_resolution_clock::now();

            // Get move
            const auto movestr = engine->go(game_clock);

            // Stop move timer
            const auto t1 = std::chrono::high_resolution_clock::now();

            // Get move time
            const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

            libataxx::Move move;

            try {
                // Parse move string
                move = parse_move(movestr);

                // Illegal move
                if (!pos.is_legal_move(move)) {
                    throw std::logic_error("Illegal move");
                }
            } catch (...) {
                info.result = make_win_for(!pos.get_turn());
                info.reason = ResultReason::IllegalMove;
                std::cout << "Illegal move \"" << movestr << "\" played by "
                          << (pos.get_turn() == libataxx::Side::Black ? game.engine1.name : game.engine2.name)
                          << "\n\n";
                break;
            }

            ply_count++;

            // Add move to .pgn
            info.history.emplace_back(move, diff.count());

            // Update clock
            if (tc.type == SearchSettings::Type::Time) {
                if (pos.get_turn() == libataxx::Side::Black) {
                    game_clock.btime -= diff.count();
                } else {
                    game_clock.wtime -= diff.count();
                }
            }

            // Out of time?
            if (tc.type == SearchSettings::Type::Movetime) {
                if (diff.count() > game_clock.movetime + adjudication.timeout_buffer) {
                    info.result = make_win_for(!pos.get_turn());
                    info.reason = ResultReason::OutOfTime;
                    break;
                }
            } else if (tc.type == SearchSettings::Type::Time) {
                if (game_clock.btime <= 0) {
                    info.result = libataxx::Result::WhiteWin;
                    info.reason = ResultReason::OutOfTime;
                    break;
                } else if (game_clock.wtime <= 0) {
                    info.result = libataxx::Result::BlackWin;
                    info.reason = ResultReason::OutOfTime;
                    break;
                }
            }

            // Increments
            if (tc.type == SearchSettings::Type::Time) {
                if (pos.get_turn() == libataxx::Side::Black) {
                    game_clock.btime += tc.binc;
                } else {
                    game_clock.wtime += tc.winc;
                }
            }

            pos.makemove(move);
        }
    } catch (...) {
        info.reason = ResultReason::EngineCrash;
        info.result = make_win_for(!pos.get_turn());
    }

    // Game finished normally
    if (info.result == libataxx::Result::None) {
        info.result = pos.get_result();
    }

    info.endpos = pos;

    return info;
}