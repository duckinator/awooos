#include "main.h"
#include "badmalloc.h"
#include <awoo.h>
#include <kernel/hal_basic_display.h>

void kernel_main(uint32_t magic, void *arg)
{
    hal_basic_display_clear();
    hal_basic_display_print(AWOO_INFO);
}