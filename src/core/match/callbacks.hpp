#ifndef CUTEATAXX_CORE_CALLBACKS_HPP
#define CUTEATAXX_CORE_CALLBACKS_HPP

#include <functional>
#include <libataxx/move.hpp>
#include <libataxx/position.hpp>
#include <string>
#include "results.hpp"

struct Callbacks {
    std::function<void(const std::string &)> on_engine_start = [](auto) {
    };
    std::function<void(const std::size_t id, const std::string &, const std::string &, const std::string &)> on_game_started =
        [](auto, auto, auto, auto) {
        };
    std::function<void(const std::size_t id, const libataxx::Result, const std::string &, const std::string &)> on_game_finished =
        [](auto, auto, auto, auto) {
        };
    std::function<void(const Results &)> on_results_update = [](auto) {
    };
    std::function<void(const std::string &)> on_info_send = [](auto) {
    };
    std::function<void(const std::string &)> on_info_recv = [](auto) {
    };
    std::function<void(const libataxx::Move &, const int)> on_move = [](auto, auto) {
    };
};

#endif
