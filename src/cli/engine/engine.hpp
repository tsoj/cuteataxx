#ifndef ENGINE_PROCESS_HPP
#define ENGINE_PROCESS_HPP

#include <boost/process.hpp>
#include <functional>
#include <libataxx/position.hpp>
#include <string>
#include <string_view>
#include "settings.hpp"

class Engine {
   public:
    virtual auto init() -> void = 0;

    [[nodiscard]] virtual auto go(const SearchSettings &settings) -> std::string = 0;

    virtual auto position(const libataxx::Position &pos) -> void = 0;

    virtual auto set_option(const std::string &name, const std::string &value) -> void = 0;

    virtual void isready() = 0;

    virtual void newgame() = 0;

   protected:
    [[nodiscard]] Engine(const std::string &path,
                         std::function<void(const std::string &msg)> send = {},
                         std::function<void(const std::string &msg)> recv = {})
        : m_child(path, boost::process::std_out > m_out, boost::process::std_in < m_in), m_send(send), m_recv(recv) {
    }

    virtual ~Engine() {
        if (is_running()) {
            m_in.close();
            m_out.close();
            m_child.wait();
        }
    }

    [[nodiscard]] auto is_running() -> bool {
        return m_child.running();
    }

    virtual void quit() = 0;

    virtual void stop() = 0;

    auto send(const std::string &msg) -> void {
        if (m_send) {
            m_send(msg);
        }
        m_in << msg << std::endl;
    }

    [[nodiscard]] auto get_output() -> std::string {
        std::string line;
        std::getline(m_out, line);
        if (m_recv) {
            m_recv(line);
        }
        return line;
    }

   private:
    boost::process::opstream m_in;
    boost::process::ipstream m_out;
    boost::process::child m_child;
    std::function<void(const std::string &msg)> m_send;
    std::function<void(const std::string &msg)> m_recv;
};

#endif
