#pragma once

#include "bot.hpp"
#include "game.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

class BotWorker {
public:
    explicit BotWorker(snake::Game& game, std::size_t bot_count, std::mutex& game_mutex)
      : game_mutex_{game_mutex} {
        bots_.reserve(bot_count);

        for(std::size_t id = 1; id <= bot_count; ++id) { bots_.emplace_back(game, id); }

        worker_ = std::jthread([this](std::stop_token st) { run(st); });
    }

    ~BotWorker() {
        worker_.request_stop();
        cv_.notify_all();
    }

    BotWorker(BotWorker const&)            = delete;
    BotWorker& operator=(BotWorker const&) = delete;

    void trigger() {
        {
            std::lock_guard lock(mutex_);
            pending_ = true;
        }

        cv_.notify_one();
    }

private:
    void run(std::stop_token st) {
        while(!st.stop_requested()) {
            std::unique_lock lock(mutex_);

            cv_.wait(lock, st, [&] { return pending_; });

            if(st.stop_requested()) break;

            pending_ = false;
            lock.unlock();
            {
                std::lock_guard game_lock(game_mutex_);
                for(auto& bot : bots_) { bot.run(); }
            }
        }
    }

    std::vector<SnakeBot> bots_;

    std::mutex                  mutex_;
    std::condition_variable_any cv_;
    bool                        pending_{false};

    std::mutex& game_mutex_;
    // Zuletzt deklariert, damit beim Zerstören
    // zuerst auf den Thread gewartet wird.
    std::jthread worker_;
};

