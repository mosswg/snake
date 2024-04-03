#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>

void clear_screen() {
	clear();
	move(0,0);
}

typedef struct {
	int16_t x, y;
} point;

void set_point(point* p, uint16_t x, uint16_t y) {
	p->x = x;
	p->y = y;
}


enum player_status {
	player_alive,
	player_dead
};

typedef struct {
	point pos, direction;
	enum player_status status;
	uint8_t length;
} player;

typedef struct {
	point size;
	player player;
	uint16_t* board;
} board;

typedef struct {
	uint8_t start_index, end_index;
	int values[256];
} buffer;

buffer input_buffer;
board game;

point random_pos() {
	point out;
	out.x = rand() % game.size.x;
	out.y = rand() % game.size.y;
	return out;
}

enum board_place_values {
	board_empty_value,
	board_apple_value,
	board_head_value,
};

uint16_t get_value_at_pos(board* b, point pos) {
	return b->board[pos.y * b->size.x + pos.x];
}

void set_value_at_pos(board* b, point pos, uint16_t value) {
	b->board[pos.y * b->size.x + pos.x] = value;
}

void generate_new_apple(board* b) {
	point new_apple = random_pos();
	if (get_value_at_pos(b, new_apple) == board_empty_value) {
		set_value_at_pos(b, new_apple, board_apple_value);
	}
}

void initialize_buffer(buffer* b) {
	b->start_index = 0;
	b->end_index = 0;
	for (int i = 0; i < 256; i++) {
		b->values[i] = 0;
	}
}

void push_to_buffer(buffer* b, int value) {
	// disallow duplicates
	if (b->values[b->end_index-1] != value) {
		b->values[b->end_index++] = value;
	}
}

int pop_from_buffer(buffer* b) {
	if (b->start_index != b->end_index) {
		return b->values[b->start_index++];
	}
	else {
		return -1;
	}
}

int tmp_x, tmp_y;
void initialize() {
	srand(time(NULL));
	initscr();
	timeout(1);
	getmaxyx(stdscr, game.size.y, game.size.x);
	game.size.x >>= 1; // divide both by 2 since we add spaces after
	game.board = calloc(game.size.x * game.size.y, sizeof(*game.board));

	set_point(&game.player.pos, game.size.x/2, game.size.y/2);
	game.player.status = player_alive;


	generate_new_apple(&game);
	game.player.length = 1;
	set_point(&game.player.direction, 0, 1);
}

void reset_game() {
	point p;
	for (p.y = 0; p.y < game.size.y; p.y++) {
		for (p.x = 0; p.x < game.size.x; p.x++) {
			set_value_at_pos(&game, p, board_empty_value);
		}
	}

	set_point(&game.player.pos, game.size.x/2, game.size.y/2);
	game.player.status = player_alive;
	generate_new_apple(&game);
	game.player.length = 1;
	set_point(&game.player.direction, 0, 1);
}

void deinitialize() {
	free(game.board);
}

#define snake_move_up ','
#define snake_move_down 'o'
#define snake_move_right 'e'
#define snake_move_left 'a'
void handle_input() {
	int c = pop_from_buffer(&input_buffer);
	switch (c) {
		case snake_move_up:
		case KEY_UP:
			if (game.player.direction.y != 1) {
				set_point(&game.player.direction, 0, -1);
			}
			break;
		case KEY_DOWN:
		case snake_move_down:
			if (game.player.direction.y != -1) {
				set_point(&game.player.direction, 0, 1);
			}
			break;
		case snake_move_left:
		case KEY_LEFT:
			if (game.player.direction.x != 1) {
				set_point(&game.player.direction, -1, 0);
			}
			break;
		case snake_move_right:
		case KEY_RIGHT:
			if (game.player.direction.x != -1) {
				set_point(&game.player.direction, 1, 0);
			}
			break;
		case 10: // Enter on reg keyboard
			printw("ENTR");
			if (game.player.status == player_dead) {
				reset_game();
			}
			break;
		case 'q':
			exit(0);
			break;
	}
}

void move_player(board* b) {
	set_value_at_pos(b, b->player.pos, board_head_value);
	b->player.pos.x += b->player.direction.x;
	b->player.pos.y += b->player.direction.y;
	if (get_value_at_pos(b, b->player.pos) == board_apple_value) {
		b->player.length++;
		generate_new_apple(b);
	}
	else if (get_value_at_pos(b, b->player.pos) != board_empty_value ||
		(b->player.pos.x > b->size.x || b->player.pos.x < 0 || b->player.pos.y > b->size.y || b->player.pos.y < 0)) {
		b->player.status = player_dead;
	}
}

const char* game_over_message = "Game Over!";
void draw_board() {
	clear_screen();
	handle_input();
	if (game.player.status == player_alive) {
		move_player(&game);
	}
	for (int y = 0; y < game.size.y; y++) {
		for (int x = 0; x < game.size.x; x++) {
			uint16_t* current_pos_value = &game.board[y * game.size.x + x];
			if (game.player.status == player_dead) {
				if (y == game.size.y / 2) {
					if (x > ((game.size.x / 2) - 6) && x < ((game.size.x / 2) + 5)) {
						addch(game_over_message[x - ((game.size.x / 2) - 5)]);
						addch(' ');
						continue;
					}
				}
			}
			if (game.player.pos.x == x && game.player.pos.y == y) {
				addch('O');
			}
			else if (*current_pos_value != board_empty_value && *current_pos_value != board_apple_value) {
				(*current_pos_value)++;
				if (*current_pos_value > (game.player.length + board_head_value)) {
					(*current_pos_value) = board_empty_value;
					addch('.');
					addch(' ');
					continue;
				}
				else {
					addch('o');
				}
			}
			else if (*current_pos_value == board_empty_value) {
				addch(' ');
			}
			else if (*current_pos_value == board_apple_value) {
				addch('@');
			}

			addch(' ');
		}
		addch('\n');
	}
	refresh();
}


#define snake_move_up ','
#define snake_move_down 'o'
#define snake_move_right 'e'
#define snake_move_left 'a'


int main() {
	clock_t start, stop;

	initialize();

	while (1) {
		start = clock();
		draw_board();

		int c;
		while ((((double)((stop = clock()) - start))/CLOCKS_PER_SEC) <= 0.003) {
			if ((c = getch()) != -1) {
				push_to_buffer(&input_buffer, c);
			}
		}
	}
	deinitialize();
}
