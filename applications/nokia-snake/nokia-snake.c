#include <furi.h>
#include <furi-hal.h>

#include <gui/gui.h>
#include <input/input.h>

typedef struct {
    // TODO: describe state here
    // TODO: потом тут будет форма змейки, однозначно определяющая отрисовываемый кадр
    uint64_t _;
} SnakeState;

static void render_callback(Canvas* canvas, void* ctx) {
    // Screen is 128x64 px

    SnakeState *snake_state = acquire_mutex((ValueMutex*)ctx, 25);

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    release_mutex((ValueMutex*)ctx, snake_state);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    osMessageQueueId_t event_queue = ctx;

    osMessageQueuePut(event_queue, input_event, 0, 0);
}

int32_t nokia_snake_app(void* p) {
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(InputEvent), NULL);

    // TODO: состояние змейки, определяющее кадр
    SnakeState snake_state;

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

// 128, 64
// (4 + 2) * 21 + 2border == 128
// 128 / 6 = 21
// (4 + 2) * 10 + 2 + 2border == 64
// 64 / 6 = 10

// snake node
// 000000
// 011110
// 011110
// 011110
// 011110
// 000000
// TODO: задать такой каждому состоянию ниже

// просто буффер 210 байт
// в нем состояния в виде цифр

/*
' ' '○'
'┏' '┓' '┗' '┛' '┃'
'╴' '╸' '╾' '━' '╼' '╺' '╶' left
'╴' '╸' '╾' '━' '╼' '╺' '╶' right
'╷' '╻' '╽' '┃' '╿' '╹' '╵' up
'╷' '╻' '╽' '┃' '╿' '╹' '╵' down
'┍' '┙' '┎' '┚' '┒' '┖' '┕' '┑'
 */

// неполные состояния используются для анимации

// вся змейка это машина состояний, применяемая к каждой клетке буфера
// только состояния '╸' '╺' '╻' '╹' вычитывают событие нажатия кнопки
// TODO: записать все возможные переходы состояний
// сначала в виде чепочек переходов, потом уже в виде асоциативного массива потом в виде switch/case

/*
┌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┐
╎┏━━━━━━━━━┓          ╎
╎┃┏╾ ○     ╵          ╎
╎┃┃                   ╎
╎┃┃                   ╎
╎┗┛                   ╎
╎                     ╎
╎       ○             ╎
╎                     ╎
╎             ○       ╎
╎                     ╎
└╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┘
*/

