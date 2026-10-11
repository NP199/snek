#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <optional>
#include <random>
#include <ranges>

using namespace std::chrono_literals;
namespace snake {

struct Position {
    std::size_t x{};
    std::size_t y{};

    friend constexpr bool operator==(Position const&, Position const&) = default;
};

enum class Direction : std::uint8_t {
    up,
    down,
    left,
    right,
};

struct Snake {
    std::vector<Position> body{};
    Direction             direction{Direction::right};
    Direction             next_direction{Direction::right};
    int                   score{};
    bool                  alive{true};
    int                   id{0};
    ftxui::Color          color{ftxui::Color::Green};
};

namespace Symbols {

    std::string Empty     = " ";
    std::string SnakeHead = "●";
    std::string SnakeBody = "█";
    std::string Food      = "◆";

}   // namespace Symbols

static constexpr std::array snake_colors{
  ftxui::Color::Green,
  ftxui::Color::Blue,
  ftxui::Color::Yellow,
  ftxui::Color::Red,
  ftxui::Color::Magenta};

struct Game {
    std::size_t                width_;
    std::size_t                height_;
    std::size_t                max_player_count_{3};
    std::size_t                player_count_{0};
    std::vector<Snake>         snakes_{};
    std::atomic<std::uint64_t> tick_{0};

    Position food_{};

    std::mt19937 random_engine_;
    bool         game_over_{false};

    std::chrono::milliseconds const fastest_speed_{20};
    std::chrono::milliseconds const slowest_speed_{120};
    std::chrono::milliseconds       speed_{slowest_speed_};

    Game(std::size_t width, std::size_t height)
      : width_{width}
      , height_{height}
      , random_engine_(std::random_device{}()) {
        snakes_.reserve(max_player_count_);
    }

    void reset() {
        //snakes_.assign(player_count_, Snake{});
        for(auto& snake : snakes_) { init_snake(snake); }
        game_over_ = false;
        speed_     = slowest_speed_;
        place_food();
    };

    using SnakeRef = std::reference_wrapper<Snake>;
    std::optional<SnakeRef> add_snake() {
        if(snakes_.size() >= max_player_count_) { return std::nullopt; }
        auto const index = snakes_.size();
        auto&      snake = snakes_.emplace_back();
        snake.color      = snake_colors[index % snake_colors.size()];
        return std::ref(snake);
    }

    bool remove_snake(Snake const& snake) {
        auto it = std::ranges::find_if(snakes_, [&snake](Snake const& s) { return &s == &snake; });

        if(it == snakes_.end()) { return false; }

        snakes_.erase(it);
        return true;
    }

    void detect_collision();

    bool is_opposite(Direction dir_now, Direction dir_command) {
        return (dir_now == Direction::up && dir_command == Direction::down)
            || (dir_now == Direction::down && dir_command == Direction::up)
            || (dir_now == Direction::left && dir_command == Direction::right)
            || (dir_now == Direction::right && dir_command == Direction::left);
    }

    void set_direction(std::size_t player, Direction direction) {
        auto& snake = snakes_.at(player);
        if(game_over_ || !snake.alive) { return; }

        if(is_opposite(snake.direction, direction)) { return; }
        snake.next_direction = direction;
    };

    void set_direction(Direction direction) { set_direction(0, direction); }

    void update() {
        if(game_over_ || snakes_.empty()) { return; }

        auto const count = snakes_.size();

        std::vector<Position> next_positions(count);
        std::vector<bool>     will_move(count, false);
        std::vector<bool>     will_eat(count, false);
        std::vector<bool>     survives(count, false);

        for(std::size_t i = 0; i < count; ++i) {
            auto& snake = snakes_.at(i);

            if(!snake.alive || snake.body.empty()) { continue; }

            snake.direction = snake.next_direction;

            Position next = snake.body.front();

            switch(snake.direction) {
            case Direction::up: --next.y; break;
            case Direction::down: ++next.y; break;
            case Direction::left: --next.x; break;
            case Direction::right: ++next.x; break;
            }

            next_positions[i] = next;
            will_move[i]      = true;
            will_eat[i]       = next == food_;
            survives[i]       = true;
        }

        bool changed;
        do {
            changed = false;

            for(std::size_t i = 0; i < count; ++i) {
                if(!survives[i]) { continue; }

                auto const next = next_positions[i];

                if(hits_wall(next) || hits_body(next, will_move, will_eat)) {
                    survives[i]  = false;
                    will_move[i] = false;
                    changed      = true;
                }
            }

            for(std::size_t i = 0; i < count; ++i) {
                for(std::size_t j = i + 1; j < count; ++j) {
                    if(!survives[i] || !survives[j]) { continue; }

                    if(next_positions[i] == next_positions[j]) {
                        survives[i]  = false;
                        survives[j]  = false;
                        will_move[i] = false;
                        will_move[j] = false;
                        changed      = true;
                    }
                }
            }
        } while(changed);

        bool food_eaten = false;

        for(std::size_t i = 0; i < count; ++i) {
            auto& snake = snakes_[i];

            if(!snake.alive || snake.body.empty()) { continue; }

            if(!survives[i]) {
                snake.alive = false;
                continue;
            }

            snake.body.insert(snake.body.begin(), next_positions[i]);

            if(will_eat[i]) {
                ++snake.score;
                food_eaten = true;
            } else {
                snake.body.pop_back();
            }
        }

        if(food_eaten) {
            place_food();
            increase_game_speed();
        }
        game_over_ = std::ranges::none_of(snakes_, [](Snake const& snake) {
            return snake.alive && !snake.body.empty();
        });
        tick_.fetch_add(1, std::memory_order_release); // Signal update complete
    }

    std::vector<Position> const& body(std::size_t player) const { return snakes_.at(player).body; }

    Position food() const { return food_; }

    bool game_over() const { return game_over_; }

    Snake const& get_snake(std::size_t player) const { return snakes_.at(player); }

    std::size_t game_speed() const {
        std::size_t percent{
          100
          - static_cast<std::size_t>(
            ((speed_ - fastest_speed_) * 99) / (slowest_speed_ - fastest_speed_))};

        return static_cast<std::size_t>(percent);
    }

    int score(std::size_t player) const { return snakes_.at(player).score; }

    int score() const { return score(0); }

    bool alive(std::size_t player) const { return snakes_.at(player).alive; }

    bool hits_wall(Position pos) const { return pos.x >= width_ || pos.y >= height_; }

    bool hits_body(Position pos) { return occupied(pos); }

    bool hits_body(
      Position                 pos,
      std::vector<bool> const& will_move,
      std::vector<bool> const& will_eat) const {
        for(std::size_t i = 0; i < snakes_.size(); ++i) {
            auto const& body = snakes_.at(i).body;

            if(body.empty()) { continue; }

            auto end = body.end();

            // Der Schwanz der bewegten Schlange verschwindet
            // im gleichen Tick, sofern kein Futter gefressen wird.
            if(will_move[i] && !will_eat[i]) { --end; }

            if(std::find(body.begin(), end, pos) != end) { return true; }
        }

        return false;
    }

    bool eats_food(Position pos) const { return pos == food_; }

    bool occupied(Position pos) const {
        return std::ranges::any_of(snakes_, [&](auto const& snake) {
            return std::ranges::find(snake.body, pos) != snake.body.end();
        });
    }

    void place_food() {
        std::uniform_int_distribution<std::size_t> x_distribution(0, width_ - 1);
        std::uniform_int_distribution<std::size_t> y_distribution(0, height_ - 1);

        do {
            food_ = Position{x_distribution(random_engine_), y_distribution(random_engine_)};
        } while(occupied(food_));
    }

    void init_snake(Snake& snake) {
        std::uniform_int_distribution<std::size_t> x_distribution(3, width_ - 3);
        std::uniform_int_distribution<std::size_t> y_distribution(3, height_ - 3);

        std::vector<Position> new_body;

        do {
            Position head{x_distribution(random_engine_), y_distribution(random_engine_)};

            new_body = {
              Position{    head.x, head.y},
              Position{head.x - 1, head.y},
              Position{head.x - 2, head.y},
            };

        } while(std::ranges::any_of(new_body, [&](Position const& pos) { return occupied(pos); }));

        snake.body           = std::move(new_body);
        snake.direction      = Direction::right;
        snake.next_direction = Direction::right;
        snake.score          = 0;
        snake.alive          = true;
        snake.id             = snakes_.size();
    }

    void increase_game_speed() {
        if(speed_ > fastest_speed_) { speed_ -= 1ms; }
    }

    void decrease_game_speed() {
        if(speed_ < slowest_speed_) { speed_ += 1ms; }
    }

    std::size_t player_count() const { return snakes_.size(); }

    std::vector<Position> occupied_positions() const {
        std::vector<Position> positions;

        for(auto const& snake : snakes_) {
            positions.insert(positions.end(), snake.body.begin(), snake.body.end());
        }

        return positions;
    }

    std::uint64_t tick() const noexcept { return tick_.load(std::memory_order_acquire); }

    void change_theme();
};

}   // namespace snake
