const std = @import("std");
const builtin = @import("builtin");
const is_test = builtin.is_test;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectError = std.testing.expectError;
const log = std.log.scoped(.x86_irq);
const build_options = @import("build_options");
const panic = @import("../../panic.zig").panic;
const idt = if (is_test) @import("../../../../test/mock/kernel/idt_mock.zig") else @import("idt.zig");
const arch = if (is_test) @import("../../../../test/mock/kernel/arch_mock.zig") else @import("arch.zig");
const pic = if (is_test) @import("../../../../test/mock/kernel/pic_mock.zig") else @import("pic.zig");
const interrupts = @import("interrupts.zig");

/// The error set for the IRQ. This will be from installing a IRQ handler.
pub const IrqError = error{
    /// The IRQ index is invalid.
    InvalidIrq,

    /// A IRQ handler already exists.
    IrqExists,
};

/// The total number of IRQ.
const NUMBER_OF_ENTRIES: u16 = 16;

/// The type of a IRQ handler. A function that takes a interrupt context and returns void.
const IrqHandler = *const fn (*arch.CpuState) usize;

// The offset from the interrupt number where the IRQs are.
pub const IRQ_OFFSET: u16 = 32;

/// The list of IRQ handlers initialised to unhandled.
var irq_handlers: [NUMBER_OF_ENTRIES]?IrqHandler = [_]?IrqHandler{null} ** NUMBER_OF_ENTRIES;

///
/// The IRQ handler that each of the IRQs will call when a interrupt happens.
///
/// Arguments:
///     IN ctx: *arch.CpuState - Pointer to the interrupt context containing the contents
///                                      of the register at the time of the interrupt.
///
export fn irqHandler(ctx: *arch.CpuState) usize {
    // Get the IRQ index, by getting the interrupt number and subtracting the offset.
    if (ctx.int_num < IRQ_OFFSET) {
        panic(@errorReturnTrace(), "Not an IRQ number: {}\n", .{ctx.int_num});
    }

    var ret_esp = @intFromPtr(ctx);

    const irq_offset = ctx.int_num - IRQ_OFFSET;
    if (isValidIrq(irq_offset)) {
        // IRQ index is valid so can truncate
        const irq_num: u8 = @truncate(irq_offset);
        if (irq_handlers[irq_num]) |handler| {
            // Make sure it isn't a spurious irq
            if (!pic.spuriousIrq(irq_num)) {
                ret_esp = handler(ctx);
                // Send the end of interrupt command
                pic.sendEndOfInterrupt(irq_num);
            }
        } else {
            panic(@errorReturnTrace(), "IRQ not registered: {}", .{irq_num});
        }
    } else {
        panic(@errorReturnTrace(), "Invalid IRQ index: {}", .{irq_offset});
    }
    return ret_esp;
}

///
/// Open an IDT entry with index and handler. This will also handle the errors.
///
/// Arguments:
///     IN index: u8                     - The IDT interrupt number.
///     IN handler: idt.InterruptHandler - The IDT handler.
///
fn openIrq(index: u8, handler: idt.InterruptHandler) void {
    idt.openInterruptGate(index, handler) catch |err| switch (err) {
        error.IdtEntryExists => {
            panic(@errorReturnTrace(), "Error opening IRQ number: {} exists", .{index});
        },
    };
}

///
/// Check whether the IRQ index is valid. This will have to be less than NUMBER_OF_ENTRIES.
///
/// Arguments:
///     IN irq_num: u8 - The IRQ index to test.
///
/// Return: bool
///     Whether the IRQ index is valid.
///
pub fn isValidIrq(irq_num: u32) bool {
    return irq_num < NUMBER_OF_ENTRIES;
}

///
/// Register a IRQ by setting its interrupt handler to the given function. This will also clear the
/// mask bit in the PIC so interrupts can happen for this IRQ.
///
/// Arguments:
///     IN irq_num: u8         - The IRQ number to register.
///     IN handler: IrqHandler - The IRQ handler to register. This is what will be called when this
///                              interrupt happens.
///
/// Errors: IrqError
///     IrqError.InvalidIrq - If the IRQ index is invalid (see isValidIrq).
///     IrqError.IrqExists  - If the IRQ handler has already been registered.
///
pub fn registerIrq(irq_num: u8, handler: IrqHandler) IrqError!void {
    // Check whether the IRQ index is valid.
    if (isValidIrq(irq_num)) {
        // Check if a handler has already been registered.
        if (irq_handlers[irq_num]) |_| {
            return IrqError.IrqExists;
        } else {
            // Register the handler and clear the PIC mask so interrupts can happen.
            irq_handlers[irq_num] = handler;
            pic.clearMask(irq_num);
        }
    } else {
        return IrqError.InvalidIrq;
    }
}

///
/// Initialise the IRQ interrupts by first remapping the port addresses and then opening up all
/// the IDT interrupt gates for each IRQ.
///
pub fn init() void {
    log.info("Init\n", .{});
    defer log.info("Done\n", .{});

    comptime var i = IRQ_OFFSET;
    inline while (i < IRQ_OFFSET + 16) : (i += 1) {
        openIrq(i, interrupts.getInterruptStub(i));
    }

    switch (build_options.test_mode) {
        .Initialisation => runtimeTests(),
        else => {},
    }
}

fn testFunction0() callconv(.Naked) void {}

fn testFunction1(ctx: *arch.CpuState) u32 {
    // Suppress unused variable warnings
    _ = ctx;
    return 0;
}

fn testFunction2(ctx: *arch.CpuState) u32 {
    // Suppress unused variable warnings
    _ = ctx;
    return 0;
}

test "openIrq" {
    idt.initTest();
    defer idt.freeTest();

    const index: u8 = 0;
    const handler = testFunction0;
    const ret: idt.IdtError!void = {};

    idt.addTestParams("openInterruptGate", .{ index, handler, ret });

    openIrq(index, handler);
}

test "isValidIrq" {
    comptime var i = 0;
    inline while (i < NUMBER_OF_ENTRIES) : (i += 1) {
        try expect(isValidIrq(i));
    }

    try expect(!isValidIrq(200));
}

test "registerIrq re-register irq handler" {
    // Set up
    pic.initTest();
    defer pic.freeTest();

    pic.addTestParams("clearMask", .{@as(u16, 0)});

    // Pre testing
    for (irq_handlers) |h| {
        try expect(null == h);
    }

    // Call function
    try registerIrq(0, testFunction1);
    try expectError(IrqError.IrqExists, registerIrq(0, testFunction2));

    // Post testing
    for (irq_handlers, 0..) |h, i| {
        if (i != 0) {
            try expect(null == h);
        } else {
            try expectEqual(testFunction1, h.?);
        }
    }

    // Clean up
    irq_handlers[0] = null;
}

test "registerIrq register irq handler" {
    // Set up
    pic.initTest();
    defer pic.freeTest();

    pic.addTestParams("clearMask", .{@as(u16, 0)});

    // Pre testing
    for (irq_handlers) |h| {
        try expect(null == h);
    }

    // Call function
    try registerIrq(0, testFunction1);

    // Post testing
    for (irq_handlers, 0..) |h, i| {
        if (i != 0) {
            try expect(null == h);
        } else {
            try expectEqual(testFunction1, h.?);
        }
    }

    // Clean up
    irq_handlers[0] = null;
}

test "registerIrq invalid irq index" {
    try expectError(IrqError.InvalidIrq, registerIrq(200, testFunction1));
}

///
/// Test that all handlers are null at initialisation.
///
fn rt_unregisteredHandlers() void {
    // Ensure all ISR are not registered yet
    for (irq_handlers, 0..) |h, i| {
        if (h) |_| {
            panic(@errorReturnTrace(), "FAILURE: Handler found for IRQ: {}-{}\n", .{ i, h });
        }
    }

    log.info("Tested registered handlers\n", .{});
}

///
/// Test that all IDT entries for the IRQs are open.
///
fn rt_openedIdtEntries() void {
    const loaded_idt = arch.sidt();
    const idt_entries = @as([*]idt.IdtEntry, @ptrFromInt(loaded_idt.base))[0..idt.NUMBER_OF_ENTRIES];

    for (idt_entries, 0..) |entry, i| {
        if (i >= IRQ_OFFSET and isValidIrq(i - IRQ_OFFSET)) {
            if (!idt.isIdtOpen(entry)) {
                panic(@errorReturnTrace(), "FAILURE: IDT entry for {} is not open\n", .{i});
            }
        }
    }

    log.info("Tested opened IDT entries\n", .{});
}

///
/// Run all the runtime tests.
///
pub fn runtimeTests() void {
    rt_unregisteredHandlers();
    rt_openedIdtEntries();
}
