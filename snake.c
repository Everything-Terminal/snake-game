#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define MAX_SNAKE_LEN     1024
#define MAX_OBSTACLES     40
#define INITIAL_DELAY_US  120000
#define GOLDEN_CHANCE     5      /* 1 in N food spawns is golden */
#define GOLDEN_LIFETIME   60     /* ticks before golden food expires */
#define HIGH_SCORE_FILE   ".snake_highscore"

typedef struct {
  int x, y;
} Point;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

enum {
  CP_SNAKE = 1,
  CP_FOOD,
  CP_GOLDEN,
  CP_WALL,
  CP_OBSTACLE,
  CP_TEXT
};

static Point snake[MAX_SNAKE_LEN];
static int snake_len;
static Direction dir;

static Point food;
static int food_is_golden;
static int golden_ticks_left;

static Point obstacles[MAX_OBSTACLES];
static int obstacle_count;

static int score;
static int high_score;
static int width, height;
static int paused;
static int use_color;

// hight score persistance

static const char *high_score_path(void) {
  static char path[512];
  const char *home = getenv("HOME");
  if (home) {
    snprintf(path, sizeof(path), "%s/%s", home, HIGH_SCORE_FILE);
  } else {
    snprintf(path, sizeof(path), "./%s", HIGH_SCORE_FILE);
  }
  return path;
}

static void load_high_score(void) {
  FILE *f = fopen(high_score_path(), "r");
  high_score = 0;
  if (f) {
    if (fscanf(f, "%d", &high_score) !=1) high_score = 0;
    fclose(f);
  }
}

static void save_high_score(void) {
  if (score <= high_score) return;
  high_score = score;
  FILE *f = fopen(high_score_path(), "w");
  if (f) {
    fprintf(f, "%d\n", high_score);
    fclose(f);
  }
}

// placement holders

static int occupied(int x, int y, int check_food) {
  for (int i = 0; i < snake_len; i++)
    if (snake[i].x == x && snake[i].y == y) return 1;
  for (int i = 0; i < obstacle_count; i++)
    if (obstacles[i].x == x && obstacles[i].y == y) return 1;
  if (check_food && food.x == x && food.y) return 1;
  return 0;
}

static void spawn_food(void) {
  do {
    food.x = rand() % (width - 2) + 1;
    food.y = rand() % (height - 2) + 1;
  } while (occupied(food.x, food.y, 0));

  food_is_golden = (rand() % GOLDEN_CHANCE == 0);
  golden_ticks_left = GOLDEN_LIFETIME;
}

static void spawn_obstacles(void) {
  obstacle_count = height / 4;
  if (obstacle_count > MAX_OBSTACLES) obstacle_count = MAX_OBSTACLES;

  for (int i = 0; i < obstacle_count; i++) {
    int x, y;
    do {
      x = rand() % (width - 2) + 1;
      y = rand() % (height - 2) + 1;
    } while (occupied(x, y, 1) ||
        (abs(x - width / 2) < 5 && abs(y - height / 2) <5));
    obstacles[i].x = x;
    obstacles[i].y = y;
  }
}

static void init_game(void) {
  getmaxyx(stdscr, height, width);
  snake_len = 3;
  dir = DIR_RIGHT;
  score = 0;
  paused = 0;

  int start_x = width / 2;
  int start_y = height / 2;
  for (int i = 0; i < snake_len; i++) {
    snake[i].x = start_x - i;
    snake[i].y = start_y;
  }

  obstacle_count = 0; /* place before food so food avoids them */
  spawn_obstacles();
  spawn_food();
}

// drawing

static void draw(void) {
  erase();

  if (use_color) attron(COLOR_PAIR(CP_WALL));
  for (int x = 0; x < width; x++) {
    mvaddch(0, x, '#');
    mvaddch(height - 1, x, '#');
  }
  for (int y = 0; y < height; y++) {
    mvaddch(y, 0, '#');
    mvaddch(y, width - 1, '#');
  }
  if (use_color) attroff(COLOR_PAIR(CP_WALL));

  if (use_color) attron(COLOR_PAIR(CP_OBSTACLE));
  for (int i = 0; i < obstacle_count; i++)
    mvaddch(obstacles[i].y, obstacles[i].x, '%');
  if (use_color) attroff(COLOR_PAIR(CP_OBSTACLE));

  if (use_color) attron(COLOR_PAIR(food_is_golden ? CP_GOLDEN : CP_FOOD));
  mvaddch(food.y, food.x, food_is_golden ? '$' : '*');
  if (use_color) attroff(COLOR_PAIR(food_is_golden ? CP_GOLDEN : CP_FOOD));

  if (use_color) attron(COLOR_PAIR(CP_SNAKE));
  for (int i = 0; i < snake_len; i++)
    mvaddch(snake[i].y, snake[i].x, i == 0 ? 'O' : 'o');
  if (use_color) attroff(COLOR_PAIR(CP_SNAKE));

  if (use_color) attron(COLOR_PAIR(CP_TEXT));
  mvprintw(0, 2, " Score: %d High: %d ", score, high_score);
  if (paused)
    mvprintw(height / 2, (width - 8) / 2, "PAUSED");
  if (use_color) attroff(COLOR_PAIR(CP_TEXT));

  refresh();
}

// input

static int handle_input(void) {
  int ch;
  while ((ch = getch()) != ERR) {
    switch (ch) {
      case 'w': case KEY_UP:
                if (dir != DIR_DOWN) dir = DIR_UP;
                break;
      case 's': case KEY_DOWN:
                if (dir != DIR_UP) dir = DIR_DOWN;
                break;
      case 'a': case KEY_LEFT:
                if (dir != DIR_RIGHT) dir = DIR_LEFT;
                break;
      case 'd': case KEY_RIGHT:
                if (dir != DIR_LEFT) dir = DIR_RIGHT;
                break;
      case 'p': paused = !paused;
                break;
      case 'q': return 0;
    }
  }
  return 1;
}

// simulation

/* return 0 if game over */
static int step(void) {
  Point next = snake[0];
  switch (dir) {
    case DIR_UP:    next.y--; break;
    case DIR_DOWN:  next.y++; break;
    case DIR_LEFT:  next.x--; break;
    case DIR_RIGHT: next.x++; break;
  }

  /* wraparound instead of dying on walls */
  if (next.x <= 0) next.x = width - 2;
  else if (next.x >= width - 1) next.x = 1;
  if (next.y <= 0) next.y = height - 2;
  else if (next.y >= height - 1) next.y = 1;

  /* obstacle collision */
  for (int i = 0; i < obstacle_count; i++) {
    if (obstacles[i].x == next.x && obstacles[i].y == next.y)
      return 0;
  }

  /* self collison */
  for (int i = 0; i < snake_len; i++) {
    if (snake[i].x == next.x && snake[i].y == next.y)
      return 0;
  }

  int grew = (next.x == food.x && next.y == food.y);

  int limit = grew ? snake_len : snake_len - 1;
  for (int i = limit; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = next;

  if (grew) {
    if (food_is_golden) {
      score += 50;
      if (snake_len + 2 < MAX_SNAKE_LEN) snake_len += 3;
    } else {
      score += 10;
      if (snake_len < MAX_SNAKE_LEN) snake_len++;
    }
    beep();
    spawn_food();
  } else if (food_is_golden) {
    if (--golden_ticks_left <= 0) spawn_food();
  }

  return 1;
}

// screens

static void game_over_screen(void) {
  save_high_score();
  nodelay(stdscr, FALSE);
  erase();
  if (use_color) attron(COLOR_PAIR(CP_TEXT));
  mvprintw(height / 2 - 1, (width - 10) / 2, "GAME OVER");
  mvprintw(height / 2, (width - 20) / 2, "Final score: %d", score);
  mvprintw(height / 2 + 1, (width - 20) / 2, "High score: %d", high_score);
  mvprintw(height / 2 + 2, (width - 26) / 2, "Press any key to exit...");
  if (use_color) attroff(COLOR_PAIR(CP_TEXT));
  refresh();
  getch();
}

// main

int main(void) {
  srand(time(NULL));
  load_high_score();

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  nodelay(stdscr, TRUE);

  use_color = has_colors();
  if (use_color) {
    start_color();
    init_pair(CP_SNAKE, COLOR_GREEN, COLOR_BLACK);
    init_pair(CP_FOOD, COLOR_RED, COLOR_BLACK);
    init_pair(CP_GOLDEN, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CP_WALL, COLOR_CYAN, COLOR_BLACK);
    init_pair(CP_OBSTACLE, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CP_TEXT, COLOR_WHITE, COLOR_BLACK);
  }

  init_game();

  int useconds = INITIAL_DELAY_US;
  int running = 1;

  while (running) {
    if (!handle_input()) break;

    if (!paused) {
      if (!step()) break;
      draw();

      useconds = INITIAL_DELAY_US - (score * 500);
      if (useconds < 50000) useconds = 50000;
    } else {
      draw();
    }

    usleep(paused ? 50000 : useconds);
  }

  game_over_screen();
  endwin();

  printf("Final score: %d (high score: %d)\n", score, high_score);
  return 0;
}
