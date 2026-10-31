#include <o2/breakout.h>

#include <math.h>
#include <raylib.h>

namespace o2::breakout {

static const float SCREEN_WIDTH = (float)WINDOW_WIDTH;
static const float SCREEN_HEIGHT = (float)WINDOW_HEIGHT;

static const float PADDLE_WIDTH = 100.0f;
static const float PADDLE_HEIGHT = 14.0f;
static const float PADDLE_Y = 560.0f;
static const float PADDLE_SPEED = 500.0f;

static const float BALL_SIZE = 12.0f;
static const float BALL_START_SPEED = 360.0f;
static const float BALL_MAX_SPEED = 640.0f;
static const float BALL_SPEED_PER_BRICK = 6.0f;
static const float LAUNCH_ANGLE = 0.5f;
static const float MAX_BOUNCE_ANGLE = 1.05f;

static const float BRICK_WIDTH = 72.0f;
static const float BRICK_HEIGHT = 20.0f;
static const float BRICK_GAP = 4.0f;
static const float BRICKS_LEFT = 22.0f;
static const float BRICKS_TOP = 60.0f;
static const int BRICK_POINTS = 10;

// The ball can't skip through a brick or the paddle as long as a frame is no longer than this.
static const float MAX_FRAME_TIME = 1.0f / 30.0f;

static const int START_LIVES = 3;
static const int TEXT_SIZE = 20;
static const int TITLE_SIZE = 40;

static const Color ROW_COLORS[BRICK_ROWS] = { RED, ORANGE, GOLD, GREEN, SKYBLUE, PURPLE };

static bool overlaps(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static float clamp(float value, float low, float high) {
	if (value < low) {
		return low;
	}
	if (value > high) {
		return high;
	}
	return value;
}

static float brickX(int column) {
	return BRICKS_LEFT + (float)column * (BRICK_WIDTH + BRICK_GAP);
}

static float brickY(int row) {
	return BRICKS_TOP + (float)row * (BRICK_HEIGHT + BRICK_GAP);
}

static void placeBallOnPaddle(Game* game) {
	game->ballX = game->paddleX + (PADDLE_WIDTH - BALL_SIZE) / 2.0f;
	game->ballY = PADDLE_Y - BALL_SIZE;
}

static void setBallAngle(Game* game, float angle) {
	game->ballVelocityX = game->ballSpeed * sinf(angle);
	game->ballVelocityY = -game->ballSpeed * cosf(angle);
}

static void movePaddle(Game* game, Input input, float dt) {
	float direction = 0.0f;
	if (input.leftDown) {
		direction -= 1.0f;
	}
	if (input.rightDown) {
		direction += 1.0f;
	}
	game->paddleX = clamp(game->paddleX + direction * PADDLE_SPEED * dt, 0.0f, SCREEN_WIDTH - PADDLE_WIDTH);
}

static void speedUpBall(Game* game) {
	float newSpeed = clamp(game->ballSpeed + BALL_SPEED_PER_BRICK, 0.0f, BALL_MAX_SPEED);
	float scale = newSpeed / game->ballSpeed;
	game->ballVelocityX *= scale;
	game->ballVelocityY *= scale;
	game->ballSpeed = newSpeed;
}

// Removes the first brick the ball is touching. Returns whether there was one.
static bool hitBrick(Game* game) {
	for (int row = 0; row < BRICK_ROWS; row++) {
		for (int column = 0; column < BRICK_COLUMNS; column++) {
			if (!game->bricks[row][column]) {
				continue;
			}
			if (!overlaps(game->ballX, game->ballY, BALL_SIZE, BALL_SIZE, brickX(column), brickY(row), BRICK_WIDTH, BRICK_HEIGHT)) {
				continue;
			}
			game->bricks[row][column] = false;
			game->bricksLeft--;
			game->score += BRICK_POINTS * (BRICK_ROWS - row);
			return true;
		}
	}
	return false;
}

// Moves one axis at a time so we know which direction to reverse when the ball hits a brick.
static void moveBall(Game* game, float dt) {
	float moveX = game->ballVelocityX * dt;
	game->ballX += moveX;
	if (game->ballX < 0.0f) {
		game->ballX = 0.0f;
		game->ballVelocityX = fabsf(game->ballVelocityX);
	} else if (game->ballX > SCREEN_WIDTH - BALL_SIZE) {
		game->ballX = SCREEN_WIDTH - BALL_SIZE;
		game->ballVelocityX = -fabsf(game->ballVelocityX);
	} else if (hitBrick(game)) {
		game->ballX -= moveX;
		game->ballVelocityX = -game->ballVelocityX;
		speedUpBall(game);
	}

	float moveY = game->ballVelocityY * dt;
	game->ballY += moveY;
	if (game->ballY < 0.0f) {
		game->ballY = 0.0f;
		game->ballVelocityY = fabsf(game->ballVelocityY);
	} else if (hitBrick(game)) {
		game->ballY -= moveY;
		game->ballVelocityY = -game->ballVelocityY;
		speedUpBall(game);
	}
}

static void bounceOffPaddle(Game* game) {
	if (game->ballVelocityY <= 0.0f) {
		return;
	}
	if (!overlaps(game->ballX, game->ballY, BALL_SIZE, BALL_SIZE, game->paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
		return;
	}
	float ballCenter = game->ballX + BALL_SIZE / 2.0f;
	float paddleCenter = game->paddleX + PADDLE_WIDTH / 2.0f;
	float offset = clamp((ballCenter - paddleCenter) / (PADDLE_WIDTH / 2.0f), -1.0f, 1.0f);
	game->ballY = PADDLE_Y - BALL_SIZE;
	setBallAngle(game, offset * MAX_BOUNCE_ANGLE);
}

static void loseBall(Game* game) {
	game->lives--;
	if (game->lives <= 0) {
		game->state = STATE_LOST;
		return;
	}
	game->ballSpeed = BALL_START_SPEED;
	game->ballVelocityX = 0.0f;
	game->ballVelocityY = 0.0f;
	placeBallOnPaddle(game);
	game->state = STATE_READY;
}

void init(Game* game) {
	game->state = STATE_READY;
	game->paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2.0f;
	game->ballSpeed = BALL_START_SPEED;
	game->ballVelocityX = 0.0f;
	game->ballVelocityY = 0.0f;
	placeBallOnPaddle(game);
	for (int row = 0; row < BRICK_ROWS; row++) {
		for (int column = 0; column < BRICK_COLUMNS; column++) {
			game->bricks[row][column] = true;
		}
	}
	game->bricksLeft = BRICK_ROWS * BRICK_COLUMNS;
	game->score = 0;
	game->lives = START_LIVES;
}

void update(Game* game, Input input, float dt) {
	bool pressed = input.leftPressed || input.rightPressed;
	dt = clamp(dt, 0.0f, MAX_FRAME_TIME);

	if (game->state == STATE_WON || game->state == STATE_LOST) {
		if (pressed) {
			init(game);
		}
		return;
	}

	movePaddle(game, input, dt);

	if (game->state == STATE_READY) {
		placeBallOnPaddle(game);
		if (pressed) {
			setBallAngle(game, input.leftPressed ? -LAUNCH_ANGLE : LAUNCH_ANGLE);
			game->state = STATE_PLAYING;
		}
		return;
	}

	moveBall(game, dt);
	if (game->bricksLeft == 0) {
		game->state = STATE_WON;
		return;
	}
	bounceOffPaddle(game);
	if (game->ballY > SCREEN_HEIGHT) {
		loseBall(game);
	}
}

static void drawCenteredText(const char* text, int y, int size, Color color) {
	DrawText(text, (WINDOW_WIDTH - MeasureText(text, size)) / 2, y, size, color);
}

void draw(const Game* game) {
	ClearBackground(BLACK);

	for (int row = 0; row < BRICK_ROWS; row++) {
		for (int column = 0; column < BRICK_COLUMNS; column++) {
			if (game->bricks[row][column]) {
				DrawRectangle((int)brickX(column), (int)brickY(row), (int)BRICK_WIDTH, (int)BRICK_HEIGHT, ROW_COLORS[row]);
			}
		}
	}

	DrawRectangle((int)game->paddleX, (int)PADDLE_Y, (int)PADDLE_WIDTH, (int)PADDLE_HEIGHT, RAYWHITE);
	if (game->state == STATE_READY || game->state == STATE_PLAYING) {
		DrawRectangle((int)game->ballX, (int)game->ballY, (int)BALL_SIZE, (int)BALL_SIZE, RAYWHITE);
	}

	DrawText(TextFormat("Score: %d", game->score), 22, 20, TEXT_SIZE, RAYWHITE);
	const char* livesText = TextFormat("Lives: %d", game->lives);
	DrawText(livesText, WINDOW_WIDTH - 22 - MeasureText(livesText, TEXT_SIZE), 20, TEXT_SIZE, RAYWHITE);

	if (game->state == STATE_READY) {
		drawCenteredText("Press LEFT or RIGHT to launch", 350, TEXT_SIZE, GRAY);
	} else if (game->state == STATE_WON) {
		drawCenteredText("YOU WIN", 300, TITLE_SIZE, GREEN);
		drawCenteredText("Press LEFT or RIGHT to play again", 350, TEXT_SIZE, GRAY);
	} else if (game->state == STATE_LOST) {
		drawCenteredText("GAME OVER", 300, TITLE_SIZE, RED);
		drawCenteredText("Press LEFT or RIGHT to play again", 350, TEXT_SIZE, GRAY);
	}
}

}
