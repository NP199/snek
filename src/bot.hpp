#pragma once

#include "game.hpp"
#include <vector>
#include <fmt/format.h>

struct snake_bot {
    std::vector<snake::Snake>                 bot_;
    std::vector<std::vector<snake::Position>> gameMap_{};
    snake::Game const&                        game_;
    bool running_ {false};

    snake_bot(std::vector<snake::Snake> const& bot, snake::Game const& game)
      : bot_{bot}
      , game_{game} {}
    ~snake_bot() { bot_.clear(); }

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

    void setNextMove() {}

    void updateSnakePosition() {}

    void run(){
        auto lastTick = game_.tick();

        while(running_){
            auto currentTick = game_.tick();

            if(currentTick == lastTick){
                continue;
            }
            lastTick = currentTick;
            updateMap();
            calculateNextMove();
            setNextMove();
        }


    }
};
