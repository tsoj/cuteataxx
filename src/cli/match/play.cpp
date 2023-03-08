#include "play.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <libataxx/pgn.hpp>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include "ataxx/adjudicate.hpp"
#include "ataxx/parse_move.hpp"
#include "engine/engine.hpp"
#include "settings.hpp"

enum class ResultReason : int
{
    Normal = 0,
    OutOfTime,
    MaterialImbalance,
    EasyFill,
    Gamelength,
    IllegalMove,
    EngineCrash,
    None,
};

[[nodiscard]] constexpr auto make_win_for(const libataxx::Side s) noexcept {
    return s == libataxx::Side::Black ? libataxx::Result::BlackWin : libataxx::Result::WhiteWin;
}

static_assert(make_win_for(libataxx::Side::Black) == libataxx::Result::BlackWin);
static_assert(make_win_for(libataxx::Side::White) == libataxx::Result::WhiteWin);

[[nodiscard]] libataxx::pgn::PGN play(const Settings &settings,
                                      const GameSettings &game,
                                      std::shared_ptr<Engine> engine1,
                                      std::shared_ptr<Engine> engine2) {
    assert(!game.fen.empty());
    assert(game.engine1.id != game.engine2.id);

    // Get engine & position settings
    auto pos = libataxx::Position{game.fen};

    // Create PGN
    libataxx::pgn::PGN pgn;
    pgn.header().add("Event", settings.pgn_event);
    pgn.header().add(settings.colour1, game.engine1.name);
    pgn.header().add(settings.colour2, game.engine2.name);
    pgn.header().add("FEN", game.fen);
    pgn.set_black_first(pos.get_turn() == libataxx::Side::Black);
    auto node = pgn.root();

    int ply_count = 0;
    int btime = settings.tc.btime;
    int wtime = settings.tc.wtime;
    auto result = libataxx::Result::None;
    auto result_reason = ResultReason::None;
    auto movestr = std::string();

    try {
        engine1->newgame();
        engine2->newgame();

        engine1->isready();
        engine2->isready();

        // Play
        while (!pos.is_gameover()) {
            // Try to adjudicate based on material imbalance
            if (settings.adjudicate_material && can_adjudicate_material(pos, *settings.adjudicate_material)) {
                result = make_win_for(pos.get_turn());
                result_reason = ResultReason::MaterialImbalance;
                break;
            }

            // Try to adjudicate based on "easy fill"
            // This is when one side has to pass and the other can fill the rest of the board trivially to win
            if (settings.adjudicate_easyfill && can_adjudicate_easyfill(pos)) {
                result = make_win_for(!pos.get_turn());
                result_reason = ResultReason::EasyFill;
                break;
            }

            // Try to adjudicate based on game length
            if (settings.adjudicate_gamelength && can_adjudicate_gamelength(pos, *settings.adjudicate_gamelength)) {
                result = libataxx::Result::Draw;
                result_reason = ResultReason::Gamelength;
                break;
            }

            auto &engine = pos.get_turn() == libataxx::Side::Black ? engine1 : engine2;

            engine->position(pos);

            engine->isready();

            auto search = settings.tc;

            // Update tc
            search.btime = btime;
            search.wtime = wtime;

            // Start move timer
            const auto t0 = std::chrono::high_resolution_clock::now();

            // Get move
            movestr = engine->go(search);

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
                result_reason = ResultReason::IllegalMove;
                result = make_win_for(!pos.get_turn());
                std::cout << "Illegal move \"" << movestr << "\" played by "
                          << (pos.get_turn() == libataxx::Side::Black ? game.engine1.name : game.engine2.name)
                          << "\n\n";
                break;
            }

            ply_count++;

            // Add move to .pgn
            node = node->add_mainline(move);

            // Comment with engine data
            if (settings.pgn_verbose) {
                node->add_comment("movetime " + std::to_string(diff.count()));
            }

            // Update clock
            if (settings.tc.type == SearchSettings::Type::Time) {
                if (pos.get_turn() == libataxx::Side::Black) {
                    btime -= diff.count();
                } else {
                    wtime -= diff.count();
                }
            }

            // Out of time?
            if (settings.tc.type == SearchSettings::Type::Movetime) {
                if (diff.count() > settings.tc.movetime + settings.timeout_buffer) {
                    result_reason = ResultReason::OutOfTime;
                    result = make_win_for(!pos.get_turn());
                    break;
                }
            } else if (settings.tc.type == SearchSettings::Type::Time) {
                if (btime <= 0) {
                    result_reason = ResultReason::OutOfTime;
                    result = libataxx::Result::WhiteWin;
                    break;
                } else if (wtime <= 0) {
                    result_reason = ResultReason::OutOfTime;
                    result = libataxx::Result::BlackWin;
                    break;
                }
            }

            // Increments
            if (settings.tc.type == SearchSettings::Type::Time) {
                if (pos.get_turn() == libataxx::Side::Black) {
                    btime += settings.tc.binc;
                } else {
                    wtime += settings.tc.winc;
                }
            }

            // Add the time left
            if (settings.pgn_verbose && settings.tc.type == SearchSettings::Type::Time) {
                if (pos.get_turn() == libataxx::Side::Black) {
                    node->add_comment("time left " + std::to_string(btime) + "ms");
                } else {
                    node->add_comment("time left " + std::to_string(wtime) + "ms");
                }
            }

            pos.makemove(move);
        }
    } catch (...) {
        result_reason = ResultReason::EngineCrash;
        result = make_win_for(!pos.get_turn());
    }

    // Game finished normally
    if (result == libataxx::Result::None) {
        result = pos.get_result();
    }

    // Add result to .pgn
    switch (result) {
        case libataxx::Result::BlackWin:
            pgn.header().add("Result", "1-0");
            pgn.header().add("Winner", game.engine1.name);
            pgn.header().add("Loser", game.engine2.name);
            break;
        case libataxx::Result::WhiteWin:
            pgn.header().add("Result", "0-1");
            pgn.header().add("Winner", game.engine2.name);
            pgn.header().add("Loser", game.engine1.name);
            break;
        case libataxx::Result::Draw:
            pgn.header().add("Result", "1/2-1/2");
            break;
        default:
            pgn.header().add("Result", "*");
            break;
    }

    switch (result_reason) {
        case ResultReason::Normal:
            break;
        case ResultReason::OutOfTime:
            pgn.header().add("Adjudicated", "Out of time");
            break;
        case ResultReason::MaterialImbalance:
            pgn.header().add("Adjudicated", "Material imbalance");
            break;
        case ResultReason::EasyFill:
            pgn.header().add("Adjudicated", "Easy fill");
            break;
        case ResultReason::Gamelength:
            pgn.header().add("Adjudicated", "Max game length reached");
            break;
        case ResultReason::IllegalMove:
            pgn.header().add("Adjudicated", "Illegal move " + movestr);
            break;
        default:
            break;
    }

    // Add some game statistics
    const auto material_difference = pos.get_black().count() - pos.get_white().count();
    pgn.header().add("PlyCount", std::to_string(ply_count));
    pgn.header().add("Final FEN", pos.get_fen());
    pgn.header().add("Material", (material_difference >= 0 ? "+" : "") + std::to_string(material_difference));

    return pgn;
}
