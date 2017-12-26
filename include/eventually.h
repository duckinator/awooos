#ifndef EVENTUALLY_H
#define EVENTUALLY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef void EvEventHandler(const char *name, void *data);

#define EVENTUALLY_MAX_EVENT_GROUPS 50
#define EVENTUALLY_MAX_HANDLERS_PER_GROUP 50

typedef struct eventually_event_group {
    const char *name;
    size_t number_of_handlers;
    EvEventHandler *handlers[EVENTUALLY_MAX_HANDLERS_PER_GROUP];
} EvEventGroup;

bool eventually_add_handler(const char *event_name, EvEventHandler *handler);
bool eventually_trigger_event(const char *event_name, void *data);

#endif
