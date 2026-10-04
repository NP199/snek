#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
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
};

namespace Symbols {

    std::string Empty     = " ";
    std::string SnakeHead = "●";
    std::string SnakeBody = "█";
    std::string Food      = "◆";

}   // namespace Symbols

struct Game {
    std::size_t        width_;
    std::size_t        height_;
    std::size_t        player_count_{1};
    std::vector<Snake> snakes_{player_count_};

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
        reset();
    }

    void reset() {
        snakes_.assign(player_count_, Snake{});
        for(auto& snake : snakes_) { init_snake(snake); }
        game_over_ = false;
        speed_     = slowest_speed_;
        place_food();
    };

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

        auto& snake = snakes_[0];

        if(!snake.alive || snake.body.empty()) { return; }

        snake.direction = snake.next_direction;

        Position next = snake.body.front();

        switch(snake.direction) {
        case Direction::up: --next.y; break;

        case Direction::down: ++next.y; break;

        case Direction::left: --next.x; break;

        case Direction::right: ++next.x; break;
        }

        if(hits_wall(next) || hits_body(next, 0)) {
            snake.alive = false;
            game_over_  = true;
            return;
        }

        snake.body.insert(snake.body.begin(), next);

        if(next == food_) {
            ++snake.score;

            place_food();
            increase_game_speed();
        } else {
            snake.body.pop_back();
        }
    }

    void update_old() {
        if(game_over_) { return; }

        auto& snake = snakes_.at(0);

        snake.direction = snake.next_direction;

        Position next = snake.body.front();

        switch(snake.direction) {
        case Direction::up: --next.y; break;

        case Direction::down: ++next.y; break;

        case Direction::left: --next.x; break;

        case Direction::right: ++next.x; break;
        }

        if(hits_wall(next) || occupied(next)) {
            game_over_  = true;
            snake.alive = false;
            return;
        }

        snake.body.insert(snake.body.begin(), next);

        if(next == food_) {
            ++snake.score;
            place_food();
            increase_game_speed();
        } else {
            snake.body.pop_back();
        }
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
    bool hits_body(Position pos, std::size_t moving_player) const {
        for(std::size_t i = 0; i < snakes_.size(); ++i) {
            auto const& body = snakes_[i].body;

            if(body.empty()) { continue; }

            auto end = body.end();

            // Der Schwanz der bewegten Schlange verschwindet
            // im gleichen Tick, sofern kein Futter gefressen wird.
            if(i == moving_player && pos != food_) { --end; }

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

    bool hits_body_after_move(Position pos, std::vector<bool> const& will_eat) const {
        for(std::size_t i = 0; i < snakes_.size(); ++i) {
            auto const& snake = snakes_[i];

            if(!snake.alive || snake.body.empty()) { continue; }

            std::size_t body_size = snake.body.size();

            // Schwanz verschwindet in diesem Tick.
            if(!will_eat[i] && body_size > 0) { --body_size; }

            auto end = snake.body.begin() + static_cast<std::ptrdiff_t>(body_size);

            if(std::find(snake.body.begin(), end, pos) != end) { return true; }
        }

        return false;
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
    }

    void increase_game_speed() {
        if(speed_ > fastest_speed_) { speed_ -= 1ms; }
    }

    void decrease_game_speed() {
        if(speed_ < slowest_speed_) { speed_ += 1ms; }
    }

    std::size_t player_count() const { return player_count_; }

    void set_player_count(std::size_t count) {
        player_count_ = std::clamp<std::size_t>(count, 1, 2);
        reset();
    }

    void change_theme();
};

}   // namespace snake
