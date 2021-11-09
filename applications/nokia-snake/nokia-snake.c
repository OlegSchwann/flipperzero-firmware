#include <furi.h>
#include <furi-hal.h>

#include <gui/gui.h>
#include <input/input.h>

typedef struct {
    //    +-----x
    //    |
    //    |
    //    y
    uint8_t x;
    uint8_t y;
} Point;

typedef enum {
    life,

    // https://melmagazine.com/en-us/story/snake-nokia-6110-oral-history-taneli-armanto
    // Armanto: While testing the early versions of the game, I noticed it was hard
    // to control the snake upon getting close to and edge but not crashing — especially
    // in the highest speed levels. I wanted the highest level to be as fast as I could
    // possibly make the device “run,” but on the other hand, I wanted to be friendly
    // and help the player manage that level. Otherwise it might not be fun to play. So
    // I implemented a little delay. A few milliseconds of extra time right before
    // the player crashes, during which she can still change the directions. And if
    // she does, the game continues.
    lastChance,

    gameOver,
    win,
} GameState;

typedef enum {
    up,
    right,
    down,
    left,
} Direction;

typedef struct {
    Point points[225];
    uint16_t len;
    Direction currentMovement;
    Direction nextMovement; // if backward of currentMovement, ignore
    Point fruit;
    GameState state;
} SnakeState;

static void render_callback(Canvas* canvas, void* ctx) {
    SnakeState* snake_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(snake_state == NULL) {
        return;
    }

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    {
        Point f = snake_state->fruit;
        f.x = f.x * 4 + 1;
        f.y = f.y * 4 + 1;
        canvas_draw_rframe(canvas, f.x, f.y, 6, 6, 2);
    }

    for(uint16_t i = 0; i < snake_state->len; i++) {
        Point p = snake_state->points[i];
        p.x = p.x * 4 + 2;
        p.y = p.y * 4 + 2;
        canvas_draw_box(canvas, p.x, p.y, 4, 4);
    }

    release_mutex((ValueMutex*)ctx, snake_state);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    osMessageQueueId_t event_queue = ctx;

    osMessageQueuePut(event_queue, input_event, 0, 0);
}

void placeNewFruit(SnakeState* snake_state, int random){
    printf("placeNewFruit\r\n");
    // 1 bit for each point on the playing field where the snake can turn and where the fruit can appear
    uint16_t buffer[8];
    memset(buffer, 0, sizeof(buffer));
    uint8_t empty = 8 * 16;

    for (uint16_t i = 0; i < snake_state->len; i++) {
        Point p = snake_state->points[i];

        if (p.x % 2 != 0 || p.y % 2 != 0) {
            continue;
        }
        p.x /= 2;
        p.y /= 2;

        buffer[p.y] |= 1 << p.x;
        empty--;
    }
    // bit set if snake use that playing field

    uint16_t newFruit = random % empty;

    for (uint8_t y = 0; y < 8; y++) {
        for (uint16_t x = 0, mask = 1; x < 16; x += 1, mask <<= 1) {
            if ((buffer[y] & mask) == 0) {
                if (newFruit == 0) {
                    snake_state->fruit.x = x * 2;
                    snake_state->fruit.y = y * 2;
                    return;
                }
                newFruit--;
            }
        }
    }
    // We will never be here
}

void process_game_step(SnakeState* snake_state) {
    bool canTurn = (snake_state->points[0].x % 2 == 0) && (snake_state->points[0].y % 2 == 0);
    if(canTurn) {
        switch(snake_state->currentMovement) {
        case up:
            switch(snake_state->nextMovement) {
            case right:
                snake_state->currentMovement = right;
                break;
            case left:
                snake_state->currentMovement = left;
            default:
                break;
            }
            break;
        case right:
            switch(snake_state->nextMovement) {
            case up:
                snake_state->currentMovement = up;
                break;
            case down:
                snake_state->currentMovement = down;
            default:
                break;
            }
            break;
        case down:
            switch(snake_state->nextMovement) {
            case right:
                snake_state->currentMovement = right;
                break;
            case left:
                snake_state->currentMovement = left;
            default:
                break;
            }
            break;
        case left:
            switch(snake_state->nextMovement) {
            case up:
                snake_state->currentMovement = up;
                break;
            case down:
                snake_state->currentMovement = down;
            default:
                break;
            }
            break;
        }
    }

    Point next_step = snake_state->points[0];
    switch(snake_state->currentMovement) {
    // +-----x
    // |
    // |
    // y
    case up:
        next_step.y--;
        break;
    case right:
        next_step.x++;
        break;
    case down:
        next_step.y++;
        break;
    case left:
        next_step.x--;
        break;
    }

    if (next_step.x == snake_state->fruit.x && next_step.y == snake_state->fruit.y) {
        snake_state->len++;
        if (snake_state->len >= 253) { // согласовать днинну буфера и максимальную выигрышную длину змейки
            snake_state->state = win;
            return;
        }

        placeNewFruit(snake_state, rand());
    }

    // TODO: написать логику коллизий
    //  со стенами,
    //  c хвостом,
    //  с фруктом. +


    memmove(
        snake_state->points + 1,
        snake_state->points,
        snake_state->len * sizeof(*(snake_state->points)));
    snake_state->points[0] = next_step;

}

void game_logic_thread(void* ctx) {
    for(;;) {
        SnakeState* snake_state = acquire_mutex((ValueMutex*)ctx, 25);
        if(snake_state == NULL) {
            continue;
        }
        process_game_step(snake_state);
        release_mutex((ValueMutex*)ctx, snake_state);

        delay(200);
    }
}

int32_t nokia_snake_app(void* p) {
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(InputEvent), NULL);

    SnakeState snake_state = {
        .points = {{8, 6}, {7, 6}, {6, 6}, {5, 6}, {4, 6}, {3, 6}, {2, 6}},
        .len = 7,
        .currentMovement = right,
        .nextMovement = right,
        .fruit = {18, 6},
        .state = life,
    };

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, &snake_state, sizeof(snake_state))) {
        printf("cannot create mutex\r\n");
        return 255;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, render_callback, &state_mutex);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // start game ligic thread
    // TODO: change to fuirac_start after same in '../music-player/music-player.c'
    osThreadAttr_t snake_logic_attr = {.name = "snake_logic_thread", .stack_size = 512};
    osThreadId_t game_logic = osThreadNew(game_logic_thread, &state_mutex, &snake_logic_attr);
    if(game_logic == NULL) {
        printf("cannot create player thread\r\n");
        return 255;
    }

    InputEvent event;
    while(1) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        SnakeState* snake_state = (SnakeState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            // press events
            if(event.type == InputTypeShort) {
                switch(event.key) {
                case InputKeyUp:
                    snake_state->nextMovement = up;
                    break;
                case InputKeyDown:
                    snake_state->nextMovement = down;
                    break;
                case InputKeyRight:
                    snake_state->nextMovement = right;
                    break;
                case InputKeyLeft:
                    snake_state->nextMovement = left;
                    break;
                case InputKeyOk:
                    break;
                case InputKeyBack:
                    release_mutex(&state_mutex, snake_state);
                    goto exit_loop;
                }
            }
        } else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, snake_state);
    }
exit_loop:

    osThreadTerminate(game_logic);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue);
    delete_mutex(&state_mutex);

    return 0;
}

// Screen is 128x64 px
// (4 + 4) * 16 - 4 + 2 + 2border == 128
// (4 + 4) * 8 - 4 + 2 + 2border == 64
// Game field from point{x:  0, y: 0} to point{x: 30, y: 14}.
// The snake turns only in even cells - intersections.
// ┌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┐
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// └╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┘
