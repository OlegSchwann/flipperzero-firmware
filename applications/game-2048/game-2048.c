#include <stdint.h>
#include <gui/gui.h>
#include <time.h>

#include "font.h"

/*
 0 empty
 1    2
 2    4
 3    8
 4   16
 5   32
 6   64
 7  128
 8  256
 9  512
10 1024
11 2048
 */
typedef uint8_t cell_state;

/* DirectionLeft <--
┌╌╌╌╌┐┌╌╌╌╌┐┌╌╌╌╌┐┌╌╌╌╌┐
╎    ╎╎    ╎╎    ╎╎    ╎
└╌╌╌╌┘└╌╌╌╌┘└╌╌╌╌┘└╌╌╌╌┘
┌╌╌╌╌┐┌╌╌╌╌┐┌╌╌╌╌┐┌╌╌╌╌┐
╎    ╎╎    ╎╎    ╎╎    ╎
└╌╌╌╌┘└╌╌╌╌┘└╌╌╌╌┘└╌╌╌╌┘
┌╌╌┌╌╌╌╌┐╌╌┐┌╌╌╌╌┐┌╌╌╌╌┐
╎ 2╎ 2  ╎  ╎╎    ╎╎    ╎
└╌╌└╌╌╌╌┘╌╌┘└╌╌╌╌┘└╌╌╌╌┘
┌╌╌┌╌╌╌╌┐┌╌╌┌╌╌╌╌┐┌╌╌╌╌┐
╎ 4╎ 4  ╎╎ 2╎ 2  ╎╎    ╎
└╌╌└╌╌╌╌┘└╌╌└╌╌╌╌┘└╌╌╌╌┘
*/
typedef enum {
    DirectionIdle,
    DirectionUp,
    DirectionRight,
    DirectionDown,
    DirectionLeft,
} Direction;

typedef struct {
    uint8_t y; // 0 <= y <= 3
    uint8_t x; // 0 <= x <= 3
} Point;

typedef struct {
    /*
    +----X
    |
    |  field[x][y]
    Y
 */
    uint8_t field[4][4];

    uint8_t next_field[4][4];

    Direction direction;
    /*
    field {
        animation-timing-function: linear;
        animation-duration: 300ms;
    }
 */
    uint32_t animation_start_ticks;

    Point keyframe_from[4][4];

    Point keyframe_to[4][4];

} GameState;


#define XtoPx(x) (33 + x * 15)

#define YtoPx(x) (1 + y * 15)

static void game_2048_render_callback(Canvas* const canvas, ValueMutex* const vm) {
    const GameState* game_state = acquire_mutex(vm, 25);
    if(game_state == NULL) {
        return;
    }

    // Before the function is called, the state is set with the canvas_reset(canvas)

    if(game_state->direction == DirectionIdle) {
        for(uint8_t y = 0; y < 4; y++) {
            for(uint8_t x = 0; x < 4; x++) {
                uint8_t field = game_state->field[y][x];
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_frame(canvas, XtoPx(x), YtoPx(y), 16, 16);
                if(field != 0) {
                    game_2048_draw_number(canvas, XtoPx(x), YtoPx(y), 1 << field);
                }
            }
        }
    } else { // if animation
        // for animation
        // (osKernelGetSysTimerCount() - game_state->animation_start_ticks) / osKernelGetSysTimerFreq();

        // TODO: end animation event/callback/set AnimationIdle
    }

    release_mutex(vm, game_state);
}

static void game_2048_input_callback(
    const InputEvent* const input_event,
    const osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    osMessageQueuePut(event_queue, input_event, 0, osWaitForever);
}

// if return false then Game Over
static bool game_2048_set_new_number(GameState *const game_state){
    uint8_t empty = 0;
    for(uint8_t y = 0; y < 4; y++) {
        for(uint8_t x = 0; x < 4; x++) {
            if (game_state->field[y][x] == 0){
                empty++;
            }
        }
    }

    if (empty == 0) {
        return false;
    }

    if (empty == 0) {
        // If it is 1 move before losing, we help the player and get rid of randomness.
        for(uint8_t y = 0; y < 4; y++) {
            for(uint8_t x = 0; x < 4; x++) {
                if (game_state->field[y][x] == 0) {
                    bool haveFour =
                    // +----X
                    // |
                    // |  field[x][y], 0 <= x, y <= 3
                    // Y
                    
                    // up == 4 or
                    (y > 0 && game_state->field[y - 1][x] == 2) ||
                    // right == 4 or
                    (x < 3 && game_state->field[y][x + 1] == 2) ||
                    // down == 4
                    (y < 3 && game_state->field[y + 1][x] == 2) ||
                    // left == 4
                    (x > 0 && game_state->field[y][x - 1] == 2);
                    

                    if (haveFour) {
                        game_state->field[y][x] = 2;
                        return true;
                    }

                    game_state->field[y][x] = 1;
                    return true;
                }
            }
        }
    }

    uint8_t target = rand() % empty;
    uint8_t twoOrFore = rand() % 4 < 3;
    for(uint8_t y = 0; y < 4; y++) {
        for(uint8_t x = 0; x < 4; x++) {
            if (target == 0) {
                if (twoOrFore) {
                    game_state->field[y][x] = 1; // 2^1 == 2  75%
                } else {
                    game_state->field[y][x] = 2; // 2^2 == 4  25%
                }
                goto exit;
            }
            target--;
        }
    }
exit:
    return true;
}

int32_t game_2048_app(void* p) {
    int32_t return_code = 0;

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(InputEvent), NULL);

    GameState* game_state = furi_alloc(sizeof(GameState));

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, game_state, sizeof(GameState))) {
        return_code = 255;
        goto free_and_exit;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(
        view_port, (ViewPortDrawCallback)game_2048_render_callback, &state_mutex);
    view_port_input_callback_set(
        view_port, (ViewPortInputCallback)game_2048_input_callback, event_queue);

    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    game_state->direction = DirectionIdle;    
    game_2048_set_new_number(game_state);
    game_2048_set_new_number(game_state);

    // <debug>
    game_state->field[0][0] = 0;
    game_state->field[0][1] = 1;
    game_state->field[0][2] = 2;
    game_state->field[0][3] = 3;

    game_state->field[1][0] = 4;
    game_state->field[1][1] = 5;
    game_state->field[1][2] = 6;
    game_state->field[1][3] = 7;
    
    game_state->field[2][0] = 8;
    game_state->field[2][1] = 9;
    game_state->field[2][2] = 10;
    game_state->field[2][3] = 11;
    
    game_state->field[3][0] = 0;
    game_state->field[3][1] = 0;
    game_state->field[3][2] = 0;
    game_state->field[3][3] = 0;
    // </debug>

    InputEvent event;
    for(bool loop = true; loop;) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);
        GameState* game_state = (GameState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyUp:;
                    break;
                case InputKeyDown:;
                    break;
                case InputKeyRight:;
                    break;
                case InputKeyLeft:;
                    break;
                case InputKeyOk:; // TODO: reinit in game ower state
                    break;
                case InputKeyBack:
                    loop = false;
                    break;
                }
            }
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, game_state);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    delete_mutex(&state_mutex);

free_and_exit:
    free(game_state);
    osMessageQueueDelete(event_queue);

    return return_code;
}
