#include <o2/breakout.h>
#include <raylib.h>

int main() {
	InitWindow(o2::breakout::WINDOW_WIDTH, o2::breakout::WINDOW_HEIGHT, "Breakout");
	SetTargetFPS(180);

	o2::breakout::Game game;
	o2::breakout::init(&game);

	while (!WindowShouldClose()) {
		o2::breakout::Input input;
		input.leftDown = IsKeyDown(KEY_LEFT);
		input.rightDown = IsKeyDown(KEY_RIGHT);
		input.leftPressed = IsKeyPressed(KEY_LEFT);
		input.rightPressed = IsKeyPressed(KEY_RIGHT);
		o2::breakout::update(&game, input, GetFrameTime());

		BeginDrawing();
		o2::breakout::draw(&game);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
