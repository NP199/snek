#include <ftxui/ftxui.hpp>

int main() {

    auto screen = ftxui::ScreenInteractive::FitComponent();

    auto renderer = ftxui::Renderer([&] {
        return ftxui::vbox({
                   ftxui::text("SNEK") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::text("Q = Beenden") | ftxui::dim | ftxui::center,
               }) |
               border;
    });

    auto component = CatchEvent(renderer, [&](ftxui::Event event) {
        if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(component);
}
