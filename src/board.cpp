#include <ftxui/ftxui.hpp>

#include <string>
#include <vector>

namespace {
constexpr int kWidth = 30;
constexpr int kHeight = 14;

ftxui::Element make_board() {
    std::vector<ftxui::Element> rows;
    rows.reserve(kHeight);

    for (int y = 0; y < kHeight; ++y) {
        std::string row(kWidth, ' ');
        rows.push_back(text(row));
    }

    return vbox(std::move(rows)) | border;
}
}  // namespace

int main() {

    auto screen = ftxui::ScreenInteractive::FitComponent();

    auto renderer = ftxui::Renderer([&] {
        return ftxui::vbox({
                   ftxui::text("SNAKE") | ftxui::bold | ftxui::center,
                   make_board(),
                   ftxui::text("Im naechsten Schritt bewegen wir einen Punkt.") | ftxui::dim,
                   ftxui::text("Q = Beenden") | ftxui::dim,
               });
    });

    auto component = CatchEvent(renderer, [&](Event event) {
        if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(component);
}
