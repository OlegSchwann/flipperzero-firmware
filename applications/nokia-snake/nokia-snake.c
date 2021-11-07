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
} point;

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

typedef struct {
    point points[225];
    uint16_t len;
    point fruit;
    GameState state;
} SnakeState;

static void render_callback(Canvas* canvas, void* ctx) {
    SnakeState *snake_state = acquire_mutex((ValueMutex*)ctx, 25);
    if (snake_state == NULL) {
        return;
    }

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    {
        point f = snake_state->fruit;
        f.x = f.x * 4 + 2;
        f.y = f.y * 4 + 2;
        canvas_draw_rbox(canvas, f.x, f.y, 6, 6, 2);
    }

    for (uint16_t i = 0; i < snake_state->len; i++) {
        point p = snake_state->points[i];
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

int32_t nokia_snake_app(void* p) {
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(InputEvent), NULL);

    // TODO: состояние змейки, определяющее кадр
    SnakeState snake_state = {
        .points = {{2,6},{3,6},{4,6},{5,6},{6,6},{7,6},{8,6}},
        .len = 7,
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

    InputEvent event;
    while(1) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        SnakeState* snake_state = (SnakeState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            // press events
            if(event.type == InputTypeShort &&
                event.key == InputKeyBack) {
                release_mutex(&state_mutex, snake_state);
                break;
            }

            if(event.type == InputTypePress &&
                event.key == InputKeyUp) {

            }

            if(event.type == InputTypePress &&
                event.key == InputKeyDown) {

            }

            if(event.type == InputTypePress &&
                event.key == InputKeyLeft) {

            }

            if(event.type == InputTypePress &&
                event.key == InputKeyRight) {

            }

            if(event.type == InputTypePress &&
               event.key == InputKeyOk) {

            }
        } else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, snake_state);
    }

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
