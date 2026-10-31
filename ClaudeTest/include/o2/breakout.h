#pragma once

namespace o2::breakout {

enum {
	WINDOW_WIDTH = 800,
	WINDOW_HEIGHT = 600,
	BRICK_ROWS = 6,
	BRICK_COLUMNS = 10,
	MAX_BALLS = 8
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
	bool upPressed;
	bool downPressed;
};

struct Ball {
	float x;
	float y;
	float velocityX;
	float velocityY;
	float speed;
};

struct Game {
	State state;
	float paddleX;
	int paddleColor;
	Ball balls[MAX_BALLS];
	int ballCount;
	bool bricks[BRICK_ROWS][BRICK_COLUMNS];
	int bricksLeft;
	// Row and column of the special brick, or -1 when there isn't one.
	int specialRow;
	int specialColumn;
	float specialTimer;
	int score;
	int lives;
};

void init(Game* game);
void update(Game* game, Input input, float dt);
void draw(const Game* game);

}
