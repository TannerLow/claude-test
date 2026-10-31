#pragma once

namespace o2::breakout {

enum {
	WINDOW_WIDTH = 800,
	WINDOW_HEIGHT = 600,
	BRICK_ROWS = 6,
	BRICK_COLUMNS = 10
};

enum State {
	STATE_READY,
	STATE_PLAYING,
	STATE_WON,
	STATE_LOST
};

struct Input {
	bool leftDown;
	bool rightDown;
	bool leftPressed;
	bool rightPressed;
};

struct Game {
	State state;
	float paddleX;
	float ballX;
	float ballY;
	float ballVelocityX;
	float ballVelocityY;
	float ballSpeed;
	bool bricks[BRICK_ROWS][BRICK_COLUMNS];
	int bricksLeft;
	int score;
	int lives;
};

void init(Game* game);
void update(Game* game, Input input, float dt);
void draw(const Game* game);

}
