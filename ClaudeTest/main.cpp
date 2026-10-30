#include <raylib.h>

int main() {
	InitWindow(800, 450, "Hello World");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("Hello World", 280, 200, 40, DARKGRAY);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
