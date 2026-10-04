#include <ftxui/ftxui.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace {
constexpr int kWidth = 30;
constexpr int kHeight = 14;

struct Position {
    int x{};
    int y{};
};

ftxui::Element make_board(Position player) {

    std::vector<ftxui::Element> rows;
    rows.reserve(kHeight);

    for (int y = 0; y < kHeight; ++y) {
        std::string row(kWidth, ' ');
        if (y == player.y) {
            row.at(static_cast<std::size_t>(player.x)) = '@';
        }
        rows.push_back(text(row));
    }

    return vbox(std::move(rows)) | border;
}
}  // namespace

int main() {

    auto screen = ScreenInteractive::FitComponent();
    Position player{kWidth / 2, kHeight / 2};

    auto renderer = Renderer([&] {
        return vbox({
                   text("SNEK - Bewegung") | bold | center,
                   make_board(player),
                   text("WASD / Pfeiltasten = bewegen | Q = Beenden") | dim,
               });
    });

    auto component = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q')) {
            screen.Exit();
            return true;
        }

        if (event == Event::ArrowUp || event == Event::Character('w')) {
            player.y = std::max(0, player.y - 1);
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('s')) {
            player.y = std::min(kHeight - 1, player.y + 1);
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::Character('a')) {
            player.x = std::max(0, player.x - 1);
            return true;
        }
        if (event == Event::ArrowRight || event == Event::Character('d')) {
            player.x = std::min(kWidth - 1, player.x + 1);
            return true;
        }

        return false;
    });

    screen.Loop(component);
}
