#include <ali/event.h>
#include <dmm.h>
#include "magic.h"
#include "multiboot.h"
#include "../src/panic.h"
#include <stddef.h>

#define MBOOT_FLAG_A_OUT 4
#define MBOOT_FLAG_ELF   5
#define MBOOT_FLAG_MMAP  6

extern size_t kernel_start;
extern size_t kernel_end;

static size_t hal_kernel_start = (size_t)&kernel_start;
static size_t hal_kernel_end   = (size_t)(&kernel_end) + 1;

void multiboot_validate_info(MultibootInfo *multiboot_info)
{
#define multiboot_flag_set(bit) ((multiboot_info->flags & (1 << bit)) != 0)

    // Multiboot a.out and ELF flags should never be set simultaneously.
    // These should never be set at the same time.
    if (multiboot_flag_set(MBOOT_FLAG_A_OUT)
            && multiboot_flag_set(MBOOT_FLAG_ELF)) {
        hal_panic("invalid multiboot header: can't have both a.out and ELF.");
    }

    // We need ELF, not a.out.
    if (multiboot_flag_set(MBOOT_FLAG_A_OUT)) {
        hal_panic("invalid multiboot header: expected ELF section, got a.out.");
    }

    // We need ELF section header information to parse STAB information.
    if (!multiboot_flag_set(MBOOT_FLAG_ELF)) {
        hal_panic("invalid multiboot header: no ELF section provided.");
    }

    // We expect a memory map to initialize DMM.
    if (!multiboot_flag_set(MBOOT_FLAG_MMAP)) {
        hal_panic("invalid multiboot header: no memory map provided.");
    }
#undef multiboot_flag_set
}

void multiboot_add_mmap_entry(MultibootMemoryMapEntry *mmap_entry)
{
    if (mmap_entry->type != MULTIBOOT_MEMORY_MAP_AVAILABLE) {
        return;
    }

    // If we get an entry starting at 0x0, make it instead start at
    // 0x1 and decrease it's length by 1 byte.
    if ((size_t)mmap_entry->addr == 0) {
        mmap_entry->addr += 1;
        mmap_entry->length -= 1;
    }

    // If we get an entry that contains the kernel, just skip
    // it for the sake of simplicity.
    //
    // An alternative approach would be to split it into two
    // entries -- e.g.,
    //     <start of actual entry> through (hal_kernel_start - 1)
    //     (hal_kernel_end + 1) through <end of actual entry>
    if (((size_t)mmap_entry->addr >= hal_kernel_start) &&
            ((size_t)mmap_entry->addr <= hal_kernel_end)) {
        return;
    }

    // If we get this far, add the memory region to DMM.
    //
    // ASSUMPTION: Despite Multiboot using uint64_t for length, assume
    //             it will always fit in a size_t.
    dmm_add_memory_region((void*)mmap_entry->addr, (size_t)mmap_entry->length);
}

void hal_multiboot_init()
{
    // Get the Multiboot info struct.
    MultibootInfo *multiboot_info = ((MultibootInfo*)hal_get_arg());

    multiboot_validate_info(multiboot_info);

    // Initialize DMM with available memory regions (as told by multiboot).
    size_t mmap_addr = multiboot_info->mmap_addr;
    size_t mmap_count = ((size_t)multiboot_info->mmap_length) / sizeof(MultibootMemoryMapEntry);
    MultibootMemoryMapEntry *mmap_entries = (MultibootMemoryMapEntry*)mmap_addr;

    for (size_t i = 0; i < mmap_count; i++) {
        multiboot_add_mmap_entry(mmap_entries + i);
    }
}

__attribute__((constructor))
void hal_multiboot_register_events()
{
    event_watch("HAL init", &hal_multiboot_init);
}