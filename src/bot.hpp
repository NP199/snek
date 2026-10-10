#pragma once

#include "game.hpp"
#include <vector>
#include <fmt/format.h>

struct snake_bot {
    std::vector<snake::Snake>                 bot_;
    std::vector<std::vector<snake::Position>> gameMap_{};
    snake::Game const&                        game_;

    snake_bot(std::vector<snake::Snake> const& bot, snake::Game const& game)
      : bot_{bot}
      , game_{game} {}
    ~snake_bot() { bot_.clear(); }

    void getMap() {}

    void updateMap() {}

    void calculateNextMove() {}

    void setNextMove() {}

    void updateSnakePosition() {}
};
