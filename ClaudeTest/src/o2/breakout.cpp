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
static const int PADDLE_COLOR_COUNT = 8;

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
static const int SPECIAL_MIN_SECONDS = 5;
static const int SPECIAL_MAX_SECONDS = 12;

// The ball can't skip through a brick or the paddle as long as a frame is no longer than this.
static const float MAX_FRAME_TIME = 1.0f / 30.0f;

static const int START_LIVES = 3;
static const int TEXT_SIZE = 20;
static const int TITLE_SIZE = 40;

static const Color ROW_COLORS[BRICK_ROWS] = { RED, ORANGE, GOLD, GREEN, SKYBLUE, PURPLE };
static const Color PADDLE_COLORS[PADDLE_COLOR_COUNT] = { RAYWHITE, RED, ORANGE, YELLOW, GREEN, SKYBLUE, BLUE, MAGENTA };
// The served ball is always the first color. Balls from the special brick take the others.
static const Color BALL_COLORS[MAX_BALLS] = { RAYWHITE, YELLOW, LIME, SKYBLUE, MAGENTA, ORANGE, RED, VIOLET };

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

static float randomSpecialDelay() {
	return (float)GetRandomValue(SPECIAL_MIN_SECONDS, SPECIAL_MAX_SECONDS);
}

static void placeBallOnPaddle(Game* game) {
	game->balls[0].x = game->paddleX + (PADDLE_WIDTH - BALL_SIZE) / 2.0f;
	game->balls[0].y = PADDLE_Y - BALL_SIZE;
}

static void setBallAngle(Ball* ball, float angle) {
	ball->velocityX = ball->speed * sinf(angle);
	ball->velocityY = -ball->speed * cosf(angle);
}

static void serveBall(Game* game) {
	game->ballCount = 1;
	game->balls[0].speed = BALL_START_SPEED;
	game->balls[0].color = 0;
	game->balls[0].velocityX = 0.0f;
	game->balls[0].velocityY = 0.0f;
	placeBallOnPaddle(game);
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

static void speedUpBall(Ball* ball) {
	float newSpeed = clamp(ball->speed + BALL_SPEED_PER_BRICK, 0.0f, BALL_MAX_SPEED);
	float scale = newSpeed / ball->speed;
	ball->velocityX *= scale;
	ball->velocityY *= scale;
	ball->speed = newSpeed;
}

// Returns the first ball color that no ball in play is using.
static int unusedBallColor(const Game* game) {
	for (int color = 0; color < MAX_BALLS; color++) {
		bool used = false;
		for (int i = 0; i < game->ballCount; i++) {
			if (game->balls[i].color == color) {
				used = true;
				break;
			}
		}
		if (!used) {
			return color;
		}
	}
	return 0;
}

// Adds a ball at the brick's position, heading down and away from the ball that hit it.
static void spawnBall(Game* game, const Ball* hitter, int row, int column) {
	if (game->ballCount >= MAX_BALLS) {
		return;
	}
	Ball* ball = &game->balls[game->ballCount];
	ball->color = unusedBallColor(game);
	game->ballCount++;
	ball->x = brickX(column) + (BRICK_WIDTH - BALL_SIZE) / 2.0f;
	ball->y = brickY(row) + (BRICK_HEIGHT - BALL_SIZE) / 2.0f;
	ball->speed = BALL_START_SPEED;
	setBallAngle(ball, hitter->velocityX > 0.0f ? -LAUNCH_ANGLE : LAUNCH_ANGLE);
	ball->velocityY = -ball->velocityY;
}

// Removes the first brick the ball is touching. Returns whether there was one.
static bool hitBrick(Game* game, Ball* ball) {
	for (int row = 0; row < BRICK_ROWS; row++) {
		for (int column = 0; column < BRICK_COLUMNS; column++) {
			if (!game->bricks[row][column]) {
				continue;
			}
			if (!overlaps(ball->x, ball->y, BALL_SIZE, BALL_SIZE, brickX(column), brickY(row), BRICK_WIDTH, BRICK_HEIGHT)) {
				continue;
			}
			game->bricks[row][column] = false;
			game->bricksLeft--;
			game->score += BRICK_POINTS * (BRICK_ROWS - row);
			if (row == game->specialRow && column == game->specialColumn) {
				game->specialRow = -1;
				game->specialColumn = -1;
				game->specialTimer = randomSpecialDelay();
				spawnBall(game, ball, row, column);
			}
			return true;
		}
	}
	return false;
}

// Moves one axis at a time so we know which direction to reverse when the ball hits a brick.
static void moveBall(Game* game, Ball* ball, float dt) {
	float moveX = ball->velocityX * dt;
	ball->x += moveX;
	if (ball->x < 0.0f) {
		ball->x = 0.0f;
		ball->velocityX = fabsf(ball->velocityX);
	} else if (ball->x > SCREEN_WIDTH - BALL_SIZE) {
		ball->x = SCREEN_WIDTH - BALL_SIZE;
		ball->velocityX = -fabsf(ball->velocityX);
	} else if (hitBrick(game, ball)) {
		ball->x -= moveX;
		ball->velocityX = -ball->velocityX;
		speedUpBall(ball);
	}

	float moveY = ball->velocityY * dt;
	ball->y += moveY;
	if (ball->y < 0.0f) {
		ball->y = 0.0f;
		ball->velocityY = fabsf(ball->velocityY);
	} else if (hitBrick(game, ball)) {
		ball->y -= moveY;
		ball->velocityY = -ball->velocityY;
		speedUpBall(ball);
	}
}

static void bounceOffPaddle(const Game* game, Ball* ball) {
	if (ball->velocityY <= 0.0f) {
		return;
	}
	if (!overlaps(ball->x, ball->y, BALL_SIZE, BALL_SIZE, game->paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
		return;
	}
	float ballCenter = ball->x + BALL_SIZE / 2.0f;
	float paddleCenter = game->paddleX + PADDLE_WIDTH / 2.0f;
	float offset = clamp((ballCenter - paddleCenter) / (PADDLE_WIDTH / 2.0f), -1.0f, 1.0f);
	ball->y = PADDLE_Y - BALL_SIZE;
	setBallAngle(ball, offset * MAX_BOUNCE_ANGLE);
}

static void loseLife(Game* game) {
	game->lives--;
	if (game->lives <= 0) {
		game->state = STATE_LOST;
		return;
	}
	serveBall(game);
	game->state = STATE_READY;
}

// Counts down while playing, then turns a random remaining brick into the special brick.
static void updateSpecialBrick(Game* game, float dt) {
	if (game->specialRow >= 0) {
		return;
	}
	game->specialTimer -= dt;
	if (game->specialTimer > 0.0f) {
		return;
	}
	int bricksToSkip = GetRandomValue(0, game->bricksLeft - 1);
	for (int row = 0; row < BRICK_ROWS; row++) {
		for (int column = 0; column < BRICK_COLUMNS; column++) {
			if (!game->bricks[row][column]) {
				continue;
			}
			if (bricksToSkip == 0) {
				game->specialRow = row;
				game->specialColumn = column;
				return;
			}
			bricksToSkip--;
		}
	}
}

void init(Game* game) {
	game->state = STATE_READY;
	game->paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2.0f;
	game->paddleColor = 0;
	serveBall(game);
	game->specialRow = -1;
	game->specialColumn = -1;
	game->specialTimer = randomSpecialDelay();
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

	if (input.upPressed) {
		game->paddleColor = (game->paddleColor + 1) % PADDLE_COLOR_COUNT;
	}
	if (input.downPressed) {
		game->paddleColor = (game->paddleColor + PADDLE_COLOR_COUNT - 1) % PADDLE_COLOR_COUNT;
	}

	if (game->state == STATE_WON || game->state == STATE_LOST) {
		if (pressed) {
			int paddleColor = game->paddleColor;
			init(game);
			game->paddleColor = paddleColor;
		}
		return;
	}

	movePaddle(game, input, dt);

	if (game->state == STATE_READY) {
		placeBallOnPaddle(game);
		if (pressed) {
			setBallAngle(&game->balls[0], input.leftPressed ? -LAUNCH_ANGLE : LAUNCH_ANGLE);
			game->state = STATE_PLAYING;
		}
		return;
	}

	updateSpecialBrick(game, dt);

	// Goes backwards so a ball spawned or swapped in this frame isn't moved until the next one.
	for (int i = game->ballCount - 1; i >= 0; i--) {
		Ball* ball = &game->balls[i];
		moveBall(game, ball, dt);
		if (game->bricksLeft == 0) {
			game->state = STATE_WON;
			return;
		}
		bounceOffPaddle(game, ball);
		if (ball->y > SCREEN_HEIGHT) {
			game->balls[i] = game->balls[game->ballCount - 1];
			game->ballCount--;
		}
	}

	if (game->ballCount == 0) {
		loseLife(game);
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
				bool special = row == game->specialRow && column == game->specialColumn;
				DrawRectangle((int)brickX(column), (int)brickY(row), (int)BRICK_WIDTH, (int)BRICK_HEIGHT, special ? WHITE : ROW_COLORS[row]);
			}
		}
	}

	DrawRectangle((int)game->paddleX, (int)PADDLE_Y, (int)PADDLE_WIDTH, (int)PADDLE_HEIGHT, PADDLE_COLORS[game->paddleColor]);
	if (game->state == STATE_READY || game->state == STATE_PLAYING) {
		for (int i = 0; i < game->ballCount; i++) {
			DrawRectangle((int)game->balls[i].x, (int)game->balls[i].y, (int)BALL_SIZE, (int)BALL_SIZE, BALL_COLORS[game->balls[i].color]);
		}
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
