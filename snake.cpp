#include <curses.h>
#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <random>
#include <mutex>
#include <queue>

using namespace std;
using namespace chrono;
using namespace chrono_literals;

namespace key {
	static constexpr int esc = 27;
	static constexpr int up = 65;
	static constexpr int down = 66;
	static constexpr int right = 67;
	static constexpr int left = 68;
} // namespace key

struct point {
	int x, y;
	bool operator==(const point& rhv) const { return this->x == rhv.x && this->y == rhv.y; }
} static head_position, fruit;

static int screen_width, screen_height;
static vector<char> screen_buffer;
static vector<point> snake;
enum { up, down, left, right } static direction;
static mt19937 rng_generator(system_clock::now().time_since_epoch().count());

point random_position()
{
	return {uniform_int_distribution<int>(0, screen_width - 1)(rng_generator),
			uniform_int_distribution<int>(0, screen_height - 1)(rng_generator)};
}

void update_screen()
{
	// Clear the buffer
	for(char& c: screen_buffer) { c = '.'; };

	// Add fruit to buffer
	screen_buffer[fruit.y * screen_width + fruit.x] = '%';

	// Add snake to buffer
	screen_buffer[snake[0].y * screen_width + snake[0].x] = '@';
	for(int i = 1; i < snake.size(); i++)
	{ screen_buffer[snake[i].y * screen_width + snake[i].x] = '#'; };

	// Draw the buffer
	clear();
	for(int y = 0; y < screen_height; y++)
	{
		for(int x = 0; x < screen_width; x++)
		{ mvaddch(y, x, screen_buffer[y * screen_width + x]); }
	}
	refresh();
}

int main()
{
	// Init curses
	initscr();
	curs_set(0);
	noecho();

	// Init variables
	getmaxyx(stdscr, screen_height, screen_width);
	screen_width = screen_width > 20 ? 20 : screen_width - 1;
	screen_height = screen_height > 20 ? 20 : screen_height - 1;

	screen_buffer = vector<char>(screen_width * screen_height, '.');
	head_position = {screen_width / 2, screen_height / 2};
	snake = vector<point> {head_position};
	fruit = random_position();

	atomic_bool game_is_running(true);
	queue<int> user_input;
	mutex user_input_lock;
	auto timer = system_clock::now();

	// Start thread that continuously updates the users input
	thread input_thread([&user_input, &user_input_lock, &game_is_running]() {
		while(game_is_running.load())
		{
			int input = getch();
			// Safely add the next input to the input queue
			{
				std::lock_guard<std::mutex> lock(user_input_lock);

				// Only add if valid input
				if(input == key::left || input == key::right || input == key::up || input == key::down)
					user_input.push(input);
			}
			this_thread::yield();
		}
	});

	// Pre-draw the screen
	update_screen();

	while(game_is_running.load())
	{
		// Update the game every 250ms
		if(system_clock::now() - timer > 250ms)
		{
			// Safely get next input in queue
			int input = -1;
			{
				std::lock_guard<std::mutex> lock(user_input_lock);
				if(user_input.size())
				{
					input = user_input.front();
					user_input.pop();
				}
			}

			if(input == key::esc)
			{
				game_is_running.store(false);
				break;
			}
			else
			{
				// Change move direction if input was given
				if(input == key::left && direction != right)
					direction = left;
				else if(input == key::right && direction != left)
					direction = right;
				else if(input == key::up && direction != down)
					direction = up;
				else if(input == key::down && direction != up)
					direction = down;
			}

			// Move in the current direction
			if(direction == up)
				head_position.y--;
			else if(direction == down)
				head_position.y++;
			else if(direction == left)
				head_position.x--;
			else if(direction == right)
				head_position.x++;

			const bool is_out_of_bounds = head_position.x < 0 || head_position.y < 0
			  || head_position.x > screen_width || head_position.y > screen_height;
			bool is_inside_itself = false;
			for(const point p: snake)
				if(p == head_position)
				{
					is_inside_itself = true;
					break;
				}

			if(is_out_of_bounds || is_inside_itself)
			{
				game_is_running.store(false);
				break;
			}
			else
			{
				// Add piece to snake if fruit was eaten and move fruit
				if(fruit == head_position)
					fruit = random_position();
				else
					snake.pop_back();

				// Move the snake
				snake.insert(snake.begin(), head_position);

				update_screen();
				timer = system_clock::now();
			}
		}

		this_thread::sleep_for(250ms);
	}

	clear();
	printw("Press any key to quit...");
	refresh();
	input_thread.join();

	endwin();
}