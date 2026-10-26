#include "bot.hpp"
#include "bot_worker.hpp"
#include "game.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/ftxui.hpp>
#include <ftxui/screen/string.hpp>
#include <memory>
#include <mutex>
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
            bool occupied = false;
            for(std::size_t i = 0; i < game.snakes_.size(); ++i) {
                auto const& snake = game.snakes_.at(i);
                auto const  it    = std::ranges::find(snake.body, pos);

                if(it != snake.body.end()) {
                    if(it == snake.body.begin()) {
                        cells.push_back(
                          ftxui::text(empty_cell) | ftxui::bgcolor(ftxui::Color::White));
                    } else {
                        cells.push_back(ftxui::text(empty_cell) | ftxui::bgcolor(snake.color));
                    }
                    occupied = true;
                    break;
                }
            }
            if(occupied) { continue; }
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
    constexpr std::size_t kBotCount{1};
    std::mutex            game_mutex;

    snake::Game               game{kWidth, kHeight};
    std::atomic<std::int64_t> tick_ms{game.speed_.count()};
    auto                      screen = ftxui::ScreenInteractive::TerminalOutput();
    std::atomic_bool          running{true};
    bool                      paused{true};
    bool                      mode{false};
    using SnakeRef = std::reference_wrapper<snake::Snake>;
    std::optional<SnakeRef> snake1{game.add_snake()};
    std::string             debugText{"Init"};
    std::optional<SnakeBot> bot;
    //std::optional<SnakeRef> snake2{};
    //std::optional<SnakeRef> snake3{};

    std::vector<std::size_t>   bot_ids;
    std::unique_ptr<BotWorker> bot_worker;

    std::uint64_t bot_version{0};

    auto renderer = ftxui::Renderer([&]() {
        std::lock_guard lock(game_mutex);

        auto board = make_board(game);

        ftxui::Element footer;

        if(game.game_over()) {
            footer
              = ftxui::text("GAME OVER | R = restart | Q = Quit") | ftxui::bold | ftxui::center;
        } else {
            footer = ftxui::vbox({
              ftxui::text(
                mode ? fmt::format("Mode: {} Players | M = 1 Player", kBotCount + 1)
                     : "Mode: 1 Player | M = Bots")
                | ftxui::bold | ftxui::center,
              ftxui::text("WASD/Arrow Keys = movement | Q = Quit") | ftxui::dim | ftxui::center,
              ftxui::hbox(
                {ftxui::text(" PageUp / PageDown = "),
                 ftxui::text(std::to_string(game.game_speed())),
                 ftxui::text(" speed[%]")})
                | ftxui::dim | ftxui::center,
              ftxui::hbox(
                ftxui::text("spacebar = pause/start "),
                ftxui::text(std::to_string(paused)))
                | ftxui::dim | ftxui::center,
              ftxui::text(debugText) | ftxui::dim | ftxui::center,

            });
        }

        return ftxui::vbox({
          ftxui::text("Snek") | ftxui::bold | ftxui::center,

          ftxui::hbox(
            {ftxui::text("Score: ") | ftxui::bold,
             ftxui::text(std::to_string(game.score())),
             ftxui::text(" | "),
             ftxui::text("Highscore: ") | ftxui::bold,
             ftxui::text(std::to_string(game.highscore_))})
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
            game.set_direction(0, snake::Direction::up);
            return true;
        }
        if(
          event == ftxui::Event::Character('S') || event == ftxui::Event::Character('s')
          || event == ftxui::Event::ArrowDown)
        {
            game.set_direction(0, snake::Direction::down);
            return true;
        }
        if(
          event == ftxui::Event::Character('A') || event == ftxui::Event::Character('a')
          || event == ftxui::Event::ArrowLeft)
        {
            game.set_direction(0, snake::Direction::left);
            return true;
        }
        if(
          event == ftxui::Event::Character('D') || event == ftxui::Event::Character('d')
          || event == ftxui::Event::ArrowRight)
        {
            game.set_direction(0, snake::Direction::right);
            return true;
        }
        if(event == ftxui::Event::Character('R') || event == ftxui::Event::Character('r')) {
            game.reset();
            paused = true;
            return true;
        }
        if(event == ftxui::Event::PageUp) {
            game.increase_game_speed();
            tick_ms.store(game.speed_.count());
            return true;
        }
        if(event == ftxui::Event::PageDown) {
            game.decrease_game_speed();
            tick_ms.store(game.speed_.count());
            return true;
        }
        if(event == ftxui::Event::Character(' ')) {
            paused = !paused;
            return true;
        }
        if(event == ftxui::Event::Character('M') || event == ftxui::Event::Character('m')) {
            /*
            mode = !mode;
            if(mode) {
                if(snake1.has_value()) { debugText = "Player 1"; }
                //bot.reset();
                //if(snake2.has_value()) { game.remove_snake(snake2->get()); }
                bot_worker.reset();
                for(std::size_t i = 0; i < bot_ids.size(); ++i) {
                    game.remove_snake(game.snakes_.back());
                }
                bot_ids.clear();
                game.reset();
            } else {
                // Bots anlegen
                for(std::size_t i = 0; i < kBotCount; ++i) {
                    auto id = game.snakes_.size();
                    game.add_snake();
                    bot_ids.push_back(id);
                }

                game.reset();

                bot_worker = std::make_unique<BotWorker>(bot_ids);

                mode = true;
                dispatch_bots();

                //bot.emplace(game);
                //snake2    = game.add_snake();
                //debugText = "Player 2";
                //game.init_snake(snake2.value());
                //snake3 = game.add_snake();
                //game.init_snake(snake3.value());
            }
            paused = true;

            return true;
            */
            paused = true;

            if(!mode) {
                // Bot-Modus aktivieren
                for(std::size_t i = 0; i < kBotCount; ++i) {
                    auto id = game.snakes_.size();

                    game.add_snake();
                    bot_ids.push_back(id);
                }

                game.reset();

                bot_worker = std::make_unique<BotWorker>(game, kBotCount, game_mutex);

                mode      = true;
                debugText = "Bots aktiviert";

            } else {
                // Bot-Modus deaktivieren
                bot_worker.reset();

                for(std::size_t i = 0; i < bot_ids.size(); ++i) {
                    game.remove_snake(game.snakes_.back());
                }

                bot_ids.clear();

                game.reset();

                mode      = false;
                debugText = "Einzelspieler";
            }

            return true;
        }
        if(event == ftxui::Event::Custom) {
            if(paused || game.game_over()) { return true; }

            {
                std::lock_guard lock(game_mutex);
                game.update();
            }

            if(bot_worker) { bot_worker->trigger(); }

            return true;
            /*
            if(paused) return true;
            //if(bot.has_value()) { bot->run(); }
            if(bot_worker) {
                auto result = bot_worker->take_result();

                if(result && result->version == bot_version) {
                    for(auto const& move : result->moves) {
                        game.set_direction(move.snake_id, move.direction);
                    }
                }
            }
            game.update();
            if(bot_worker && !game.game_over()) { dispatch_bots(); }
            return true;
        }
        */
        }
        return false;
    });

    game.reset();   //first init to get snake

    std::jthread ticker([&](std::stop_token stop_token) {
        while(!stop_token.stop_requested() && running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{tick_ms.load()});
            if(!stop_token.stop_requested() && running.load()) {
                screen.PostEvent(ftxui::Event::Custom);
            }
        }
    });

    screen.Loop(component);

    running = false;
    ticker.request_stop();
}
