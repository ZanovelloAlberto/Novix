const ALIGN = 1 << 0;
const MEMINFO = 1 << 1;
const MAGIC = 0x1BADB002;
const FLAGS = ALIGN | MEMINFO;
const std = @import("std");
const MultibootHeader = packed struct {
    magic: i32 = MAGIC,
    flags: i32,
    checksum: i32,
    padding: u32 = 0,
};
extern var KERNEL_VADDR_END: *u32;

/// The virtual start of the kernel code.
extern var KERNEL_VADDR_START: *u32;

/// The physical end of the kernel code.
extern var KERNEL_PHYSADDR_END: *u32;

/// The physical start of the kernel code.
extern var KERNEL_PHYSADDR_START: *u32;

/// The boot-time offset that the virtual addresses are from the physical addresses.
extern var KERNEL_ADDR_OFFSET: *u32;

/// The virtual address of the top limit of the stack.
extern var KERNEL_STACK_START: *u32;

/// The virtual address of the base of the stack.
extern var KERNEL_STACK_END: *u32;

export var multiboot: MultibootHeader align(4) linksection(".multiboot") = .{
    .flags = FLAGS,
    .checksum = -(MAGIC + FLAGS),
};

var stack_bytes: [16 * 1024]u8 align(16) linksection(".bss") = undefined;

// We specify that this function is "naked" to let the compiler know
// not to generate a standard function prologue and epilogue, since
// we don't have a stack yet.
export fn _start() callconv(.Naked) noreturn {
    // We use inline assembly to set up the stack before jumping to
    // our kernel main.
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
        : [stack_top] "i" (@as([*]align(16) u8, @ptrCast(&stack_bytes)) + @sizeOf(@TypeOf(stack_bytes))),
          // We let the compiler handle the reference to kmain by passing it as an input operand as well.
          [kmain] "X" (&kmain),
    );
}

const console = @import("./console.zig");

pub fn customLog(
    comptime level: std.log.Level,
    comptime scope: @TypeOf(.EnumLiteral),
    comptime format: []const u8,
    args: anytype,
) void {
    const prefix = "[{s} {s}]: ";
    const scope_str = if (scope == .default) "default" else @tagName(scope);
    console.color = console.vgaEntryColor(switch (level) {
        .err => .Red,
        .warn => .Yellow,
        .info => .Blue,
        .debug => .Green,
    }, .Black);
    console.printf(prefix, .{ @tagName(level), scope_str });
    console.color = console.vgaEntryColor(.White, .Black);
    // _ = format;
    // _ = args;
    console.printf(format ++ "\n", args);
}
pub const std_options: std.Options = .{
    .logFn = customLog,
};

fn kmain() callconv(.C) void {
    console.initialize();
    std.log.err("initilize", .{});
    std.log.info("deinit {}", .{10});
    // std.log.info("initlia {s}", .{"peppere"});
    // console.printf("valuffan\n", .{});
    // console.printf("valuffan\n", .{});
    // console.printf("valuffan\n", .{});
    // tty.init(alloc: Allocator, boot_payload: arch.BootPayload)
    // console.initialize();
    // console.puts("Hello Zig Kernel!  ");
    // console.printf("perro   {x}  ", .{0xBEEF});
    // console.printf("start:  0x{x}  ", .{@intFromPtr(KERNEL_VADDR_START)});
    // console.printf("start:  0x{x}  ", .{@intFromPtr(KERNEL_ADDR_OFFSET)});
    // console.printf("start:  0x{x}  ", .{@intFromPtr(KERNEL_VADDR_END)});
    // console.printf("end:    0x{x}  ", .{@intFromPtr(KERNEL_PHYSADDR_END)});
    while (true) {
        std.log.debug("looping", .{});
    }
}
