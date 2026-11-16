#include "ChessUI.hpp"
#include "raylib.h"

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(ChessUI::WINDOW_WIDTH, ChessUI::WINDOW_HEIGHT, "Chess - vcpkg + MSVC + CMake");
    SetTargetFPS(60);

    ChessUI ui;
    while (!WindowShouldClose()) {
        ui.Update();
        ui.Draw();
    }

    CloseWindow();
    return 0;
}