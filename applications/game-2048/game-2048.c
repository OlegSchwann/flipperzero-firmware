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

static void game_2048_render_callback(Canvas *const canvas, ValueMutex *const vm) {
    const GameState* game_state = acquire_mutex(vm, 25);
    if(game_state == NULL) {
        return;
    }

    // Before the function is called, the state is set with the canvas_reset(canvas)

    canvas_draw_frame(canvas, 0, 0, 16, 16);
    DrawNumberFor2048(canvas, 30, 30, 64);
    DrawNumberFor2048(canvas, 40, 40, 1024);

    // for animation
    // (osKernelGetSysTimerCount() - game_state->animation_start_ticks) / osKernelGetSysTimerFreq();

    release_mutex(vm, game_state);
}

static void game_2048_input_callback(const InputEvent *const input_event, const osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    osMessageQueuePut(event_queue, input_event, 0, osWaitForever);
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
    view_port_draw_callback_set(view_port, (ViewPortDrawCallback)game_2048_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, (ViewPortInputCallback)game_2048_input_callback, event_queue);

    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;
    for(bool loop = true; loop;) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);
        GameState* game_state = (GameState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyUp:
                    ;
                    break;
                case InputKeyDown:
                    ;
                    break;
                case InputKeyRight:
                    ;
                    break;
                case InputKeyLeft:
                    ;
                    break;
                case InputKeyOk:
                    ; // TODO: reinit in game ower state 
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
