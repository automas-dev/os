#ifndef EBUS_H
#define EBUS_H

#include <stddef.h>
#include <stdint.h>

#include "libc/datastruct/array.h"
#include "libc/datastruct/circular_buffer.h"

/*
    Events

    - Keyboard
    - Timer
    - Signals
    - File I/O
    - Network I/O
    - ATA I/O
    - Custom

*/

enum EBUS_EVENT {
    EBUS_EVENT_ANY = 0,
    EBUS_EVENT_TIMER,
    EBUS_EVENT_KEY,
    EBUS_EVENT_EXEC,
    EBUS_EVENT_PROC_MADE,
    EBUS_EVENT_PROC_CLOSE,
    EBUS_EVENT_CUSTOM,
};

typedef struct _ebus_event {
    int event_id;
    int source_pid;
    union {
        struct {
            int      id;
            uint32_t time;
        } timer;
        struct {
            uint8_t  event;
            uint8_t  mods;
            char     c;
            uint32_t keycode;
            uint32_t scancode;
        } key;
        struct {
            const char * filename;
            size_t       argc;
            char **      argv;
        } exec;
        struct {
            int pid;
            int error; // TODO use this
        } proc_made;
        struct {
            int pid;
            int status_code;
        } proc_close;
        struct {
            int    id;
            void * data;
            size_t size;
        } custom;
    };
} ebus_event_t;

typedef void (*ebus_handler_fn)(const ebus_event_t * event);

typedef struct _ebus_handler {
    int             event_id;
    int             id;
    int             pid;
    ebus_handler_fn callback_fn;
} ebus_handler_t;

typedef struct _ebus {
    arr_t handlers; // ebus_handler_t
    cb_t  queue;    // ebus_event_t

    int next_handler_id;
} ebus_t;

int ebus_create(ebus_t * bus, size_t event_queue_size);

void ebus_free(ebus_t * bus);

int ebus_queue_size(ebus_t * bus);

/**
 * @brief
 *
 * @param bus
 * @param handler
 * @return int id, <= 0 for failure
 */
int  ebus_register_handler(ebus_t * bus, ebus_handler_t * handler);
void ebus_unregister_handler(ebus_t * bus, int handler_id);

int ebus_push(ebus_t * bus, ebus_event_t * event);
int ebus_pop(ebus_t * bus, ebus_event_t * event_out);

// int ebus_cycle(ebus_t * bus);

#endif // EBUS_H
