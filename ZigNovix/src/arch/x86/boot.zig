const arch = @import("arch.zig");

/// The multiboot header
const MultiBoot = packed struct {
    magic: i32,
    flags: i32,
    checksum: i32,
};

const ALIGN = 1 << 0;
const MEMINFO = 1 << 1;
const MAGIC = 0x1BADB002;
const FLAGS = ALIGN | MEMINFO;

const KERNEL_PAGE_NUMBER = 0xC0000000 >> 22;
// The number of pages occupied by the kernel, will need to be increased as we add a heap etc.
const KERNEL_NUM_PAGES = 1;

// export var multiboot align(4) linksection(".rodata.boot") = MultiBoot{
//     .magic = MAGIC,
//     .flags = FLAGS,
//     .checksum = -(MAGIC + FLAGS),
// };

const MultibootHeader = packed struct {
    magic: i32 = MAGIC,
    flags: i32,
    checksum: i32,
    padding: u32 = 0,
};

export var multiboot: MultibootHeader align(4) linksection(".rodata.boot") = .{
    .flags = FLAGS,
    .checksum = -(MAGIC + FLAGS),
};

// The initial page directory used for booting into the higher half. Should be overwritten later
export var boot_page_directory: [1024]u32 align(4096) linksection(".rodata.boot") = init: {
    // Increase max number of branches done by comptime evaluator
    @setEvalBranchQuota(1024);
    // Temp value
    var dir: [1024]u32 = undefined;

    // Page for 0 -> 4 MiB. Gets unmapped later
    dir[0] = 0x00000083;

    var i = 0;
    var idx = 1;

    // Fill preceding pages with zeroes. May be unnecessary but incurs no runtime cost
    while (i < KERNEL_PAGE_NUMBER - 1) : ({
        i += 1;
        idx += 1;
    }) {
        dir[idx] = 0;
    }

    // Map the kernel's higher half pages increasing by 4 MiB every time
    i = 0;
    while (i < KERNEL_NUM_PAGES) : ({
        i += 1;
        idx += 1;
    }) {
        dir[idx] = 0x00000083 | (i << 22);
    }
    // Fill succeeding pages with zeroes. May be unnecessary but incurs no runtime cost
    i = 0;
    while (i < 1024 - KERNEL_PAGE_NUMBER - KERNEL_NUM_PAGES) : ({
        i += 1;
        idx += 1;
    }) {
        dir[idx] = 0;
    }
    break :init dir;
};

export var kernel_stack: [16 * 1024]u8 align(16) linksection(".bss.stack") = undefined;
extern var KERNEL_ADDR_OFFSET: *u32;

extern fn kmain(mb_info: arch.BootPayload) void;

export fn _start() align(16) linksection(".text.boot") callconv(.Naked) noreturn {
    // Set the page directory to the boot directory
    asm volatile (
        \\.extern boot_page_directory
        \\mov $boot_page_directory, %%ecx
        \\mov %%ecx, %%cr3
    );

    // Enable 4 MiB pages
    asm volatile (
        \\mov %%cr4, %%ecx
        \\or $0x00000010, %%ecx
        \\mov %%ecx, %%cr4
    );

    // Enable paging
    asm volatile (
        \\mov %%cr0, %%ecx
        \\or $0x80000000, %%ecx
        \\mov %%ecx, %%cr0
    );
    asm volatile ("jmp start_higher_half");
    while (true) {}
}

export fn start_higher_half() callconv(.naked) noreturn {
    // Invalidate the page for the first 4MiB as it's no longer needed
    asm volatile ("invlpg (0)");

    // Setup the stack
    // asm volatile (
    //     \\.extern KERNEL_STACK_END
    //     \\mov $KERNEL_STACK_END, %%esp
    //     \\sub $32, %%esp
    //     \\mov %%esp, %%ebp
    // );

    // // Get the multiboot header address and add the virtual offset
    // const mb_info_addr = asm (
    //     \\mov %%ebx, %[res]
    //     : [res] "=r" (-> usize),
    // ) + @intFromPtr(&KERNEL_ADDR_OFFSET);

    // const v: *anyopaque = @ptrFromInt(mb_info_addr);
    // kmain(v);
    // while (true) {}

    asm volatile (
        \\ movl %[stack_top], %%esp
        \\ movl %%esp, %%ebp
        \\ call %[kmain:P]
        :
        // The stack grows downwards on x86, so we need to point ESP
        // to one element past the end of `stack_bytes`.
        //
        // Unfortunately, we can't just compute `&stack_bytes[stack_bytes.len]`,
        // as the Zig compiler will notice the out-of-bounds access
        // at compile-time and throw an error.
        //
        // We can instead take the start address of `stack_bytes` and
        // add the size of the array to get the one-past-the-end
        // pointer. However, Zig disallows pointer arithmetic on all
        // pointer types except "multi-pointers" `[*]`, so we must cast
        // to that type first.
        //
        // Finally, we pass the whole expression as an input operand
        // with the "immediate" constraint to force the compiler to
        // encode this as an absolute address. This prevents the
        // compiler from doing unnecessary extra steps to compute
        // the address at runtime (especially in Debug mode), which
        // could possibly clobber registers that are specified by
        // multiboot to hold special values (e.g. EAX).
        : [stack_top] "i" (@as([*]align(16) u8, @ptrCast(&kernel_stack)) + @sizeOf(@TypeOf(kernel_stack))),
          // We let the compiler handle the reference to kmain by passing it as an input operand as well.
          [kmain] "X" (&kmain),
    );
}
