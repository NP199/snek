#pragma once

#include "game.hpp"

#include <fmt/format.h>
#include <optional>
#include <vector>

struct SnakeBot {
    std::vector<std::vector<snake::Position>> gameMap_{};
    snake::Game&                              game_;
    bool                                      running_{true};
    using SnakeRef = std::reference_wrapper<snake::Snake>;
    std::optional<SnakeRef> bot_snake_{};

    SnakeBot(snake::Game& game) : game_{game} { bot_snake_ = game_.add_snake(); }
    ~SnakeBot() {
        if(bot_snake_.has_value()) { game_.remove_snake(bot_snake_->get()); }
        bot_snake_.reset();
    }

    void updateMap() {
        std::vector<snake::Position> occupiedPos = game_.occupied_positions();

        // Karte aktualisieren
        // Snakes merken // head // direction // possible next direction
        // Food Pos merken
    }

    void calculateNextMove() {
        // Weg länge zu food berechnen | ich | die anderen
        // wenn erster, dann zum food
        // wenn zweiter || auf angriff wechseln
        // weg danach möglich?
        // head und body andere schlange
        // andere schlange angreifbar?
    }

    void setNextMove() {
        if(bot_snake_.has_value()) {
            auto const& temp = bot_snake_->get();
            game_.set_direction(1, snake::Direction::up);
            if(temp.direction == snake::Direction::left) {
                game_.set_direction(1, snake::Direction::down);
            } else if(temp.direction == snake::Direction::right) {
                //temp.direction = snake::Direction::up;
                game_.set_direction(1, snake::Direction::up);
            } else if(temp.direction == snake::Direction::up) {
                game_.set_direction(1, snake::Direction::left);
            } else if(temp.direction == snake::Direction::down) {
                game_.set_direction(1, snake::Direction::right);
            }
        }
    }

    void updateSnakePosition() {}

    void run() {
        auto lastTick     = game_.tick();
        auto nextMoveTick = lastTick;

        //game_.set_direction(1, snake::Direction::up);
        setNextMove();
        /*while(running_) {
            auto currentTick = game_.tick();

            if(currentTick == lastTick) { continue; }
            lastTick = currentTick;
            updateMap();
            calculateNextMove();
            if(currentTick == nextMoveTick) {
                setNextMove();
                nextMoveTick += 2;
            }
        }*/
    }
};
