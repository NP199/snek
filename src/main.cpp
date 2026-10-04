#include "game.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/ftxui.hpp>
#include <ftxui/screen/string.hpp>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

ftxui::Element make_board(snake::Game const& game) {
    constexpr std::size_t kCellWidth = 2;

    std::string const empty_cell(kCellWidth, ' ');

    std::vector<ftxui::Element> rows;
    rows.reserve(game.height_);

    for(std::size_t y = 0; y < game.height_; ++y) {
        ftxui::Elements cells;
        cells.reserve(game.width_);

        for(std::size_t x = 0; x < game.width_; ++x) {
            snake::Position const pos{x, y};

            if(pos == game.food()) {
                cells.push_back(ftxui::text(empty_cell) | ftxui::bgcolor(ftxui::Color::Red));
                continue;
            }

            auto const& body_1 = game.body(0);
            auto const  it_1   = std::ranges::find(body_1, pos);

            if(it_1 != body_1.end()) {
                if(it_1 == body_1.begin()) {
                    cells.push_back(ftxui::text(empty_cell) | ftxui::bgcolor(ftxui::Color::White));
                } else {
                    cells.push_back(ftxui::text(empty_cell) | ftxui::bgcolor(ftxui::Color::Green));
                }

                continue;
            }
            if(game.player_count() == 2) {
                auto const& body_2 = game.body(1);
                auto const  it_2   = std::ranges::find(body_2, pos);

                if(it_2 != body_2.end()) {
                    if(it_2 == body_2.begin()) {
                        cells.push_back(
                          ftxui::text(empty_cell) | ftxui::bgcolor(ftxui::Color::White));
                    } else {
                        cells.push_back(
                          ftxui::text(empty_cell) | ftxui::bgcolor(ftxui::Color::Blue));
                    }

                    continue;
                }
            }

            cells.push_back(ftxui::text(empty_cell));
        }

        rows.push_back(ftxui::hbox(std::move(cells)));
    }

    constexpr std::size_t kBorderSize = 2;

    auto const board_width = game.width_ * kCellWidth + kBorderSize;

    auto const board_height = game.height_ + kBorderSize;

    return ftxui::vbox(std::move(rows))
         | ftxui::border

         // Größe inklusive Border festsetzen!
         | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(board_width))

         | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<int>(board_height));
}

int main() {
    constexpr std::size_t kWidth{15};
    constexpr std::size_t kHeight{15};

    snake::Game      game{kWidth, kHeight};
    auto             screen = ftxui::ScreenInteractive::TerminalOutput();
    std::atomic_bool running{true};
    bool             paused{true};
    bool             mode{false};

    auto renderer = ftxui::Renderer([&]() {
        auto board = make_board(game);

        ftxui::Element footer;

        if(game.game_over()) {
            footer
              = ftxui::text("GAME OVER | R = restart | Q = Quit") | ftxui::bold | ftxui::center;
        } else {
            footer = ftxui::vbox({
              ftxui::text(
                mode ? "Mode: 2 Players | M = 1 Player" : "Mode: 1 Player | M = 2 Players")
                | ftxui::bold | ftxui::center,
              ftxui::text("WASD/Arrow Keys = movement | Q = Quit") | ftxui::dim | ftxui::center,
              ftxui::hbox(
                {ftxui::text(" PageUp / PageDown = "),
                 ftxui::text(std::to_string(game.game_speed())),
                 ftxui::text(" speed[%]")})
                | ftxui::dim | ftxui::center,
              ftxui::text("spacebar = pause/start") | ftxui::dim | ftxui::center,
            });
        }

        return ftxui::vbox({
          ftxui::text("Snek") | ftxui::bold | ftxui::center,

          ftxui::hbox(
            {ftxui::text("Score: ") | ftxui::bold, ftxui::text(std::to_string(game.score()))})
            | ftxui::center,

          board | ftxui::center,

          footer,
        });
    });

    auto component = ftxui::CatchEvent(renderer, [&](ftxui::Event event) {
        if(event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) {
            running = false;
            screen.Exit();
            return true;
        }
        if(
          event == ftxui::Event::Character('W') || event == ftxui::Event::Character('w')
          || event == ftxui::Event::ArrowUp)
        {
            game.set_direction(snake::Direction::up);
            return true;
        }
        if(
          event == ftxui::Event::Character('S') || event == ftxui::Event::Character('s')
          || event == ftxui::Event::ArrowDown)
        {
            game.set_direction(snake::Direction::down);
            return true;
        }
        if(
          event == ftxui::Event::Character('A') || event == ftxui::Event::Character('a')
          || event == ftxui::Event::ArrowLeft)
        {
            game.set_direction(snake::Direction::left);
            return true;
        }
        if(
          event == ftxui::Event::Character('D') || event == ftxui::Event::Character('d')
          || event == ftxui::Event::ArrowRight)
        {
            game.set_direction(snake::Direction::right);
            return true;
        }
        if(event == ftxui::Event::Character('R') || event == ftxui::Event::Character('r')) {
            game.reset();
            return true;
        }
        if(event == ftxui::Event::PageUp) {
            game.increase_game_speed();
            return true;
        }
        if(event == ftxui::Event::PageDown) {
            game.decrease_game_speed();
            return true;
        }
        if(event == ftxui::Event::Character(' ')) {
            paused = !paused;
            return true;
        }
        if(event == ftxui::Event::Character('M') || event == ftxui::Event::Character('m')) {
            mode = !mode;
            if(mode) {
                game.set_player_count(1);
            } else {
                game.set_player_count(2);
            }
            paused = true;

            return true;
        }

        if(event == ftxui::Event::Custom) {
            if(!paused) game.update();
            return true;
        }
        return false;
    });

    std::jthread ticker([&](std::stop_token stop_token) {
        while(!stop_token.stop_requested() && running.load()) {
            std::this_thread::sleep_for(game.speed_);
            if(!stop_token.stop_requested() && running.load()) {
                screen.PostEvent(ftxui::Event::Custom);
            }
        }
    });

    screen.Loop(component);

    running = false;
    ticker.request_stop();
}
