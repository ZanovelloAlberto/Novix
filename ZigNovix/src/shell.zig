// Copyright (C) 2025, Novice
//
// This file is part of the Novix software.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

const std = @import("std");

// External declarations (assuming these are defined elsewhere)
extern fn putc(c: u8) void;
extern fn getchar() u8;
extern fn printf(comptime format: []const u8, args: anytype) void;
extern fn puts(s: []const u8) void;
extern fn VGA_clr() void;
extern fn VGA_moveCursorTo(line: c_int, col: c_int) void;
extern fn VGA_coloredPuts(s: []const u8, color: c_int) void;
extern fn VGA_getCurrentLine() c_int;
extern fn panic() void;
extern fn waitForKeyPress() void;
extern fn FDC_readSectors(buffer: [*]u8, sector: u16, count: c_int) void;
extern fn VFS_open(path: []const u8, flags: c_int) c_int;
extern fn VFS_read(fd: c_int, buffer: [*]u8, size: usize) usize;
extern fn VFS_close(fd: c_int) void;
extern fn VIRTMEM_mapPage(addr: *anyopaque, writable: bool) void;
extern fn switch_to_usermode(entry: usize, stack: usize) void;

const console = @import("./console.zig");

const physmem_info_t = extern struct {
    bitmapSize: c_int,
    totalBlockNumber: c_int,
    totalFreeBlock: c_int,
    totalUsedBlock: c_int,
};

extern fn PHYSMEM_getMemoryInfo(info: *physmem_info_t) void;

// Constants
const MAX_CHAR_PROMPT = 256;
const MAX_CMD_ARGS = 64;
const VGA_COLOR_LIGHT_CYAN = 11; // Assuming a value, adjust as needed

// Enums
const QuoteFlagState = enum {
    Free,
    WaitForSingleQuote,
    WaitForDoubleQuote,
};

// Global variables
var prompt: [MAX_CHAR_PROMPT]u8 = undefined;
var args: [MAX_CMD_ARGS]?[*]u8 = [_]?[*]u8{null} ** MAX_CMD_ARGS;
var argc: c_int = 0;

// Private functions
fn promptPurify(source: []u8) void {
    var space_count: u16 = 0;
    var index: usize = 0;

    // Count leading whitespace
    while (index < source.len and (source[index] == ' ' or source[index] == '\t')) : (index += 1) {
        space_count += 1;
    }

    // Shift string to remove leading spaces
    var i: usize = 0;
    while (i + space_count < source.len and source[i + space_count] != 0) : (i += 1) {
        source[i] = source[i + space_count];
    }
    source[i] = 0;

    // Trim trailing spaces
    if (i > 0) {
        index = i - 1;
        while (index > 0 and (source[index] == ' ' or source[index] == '\t')) : (index -= 1) {
            source[index] = 0;
        }
    }
}

fn promptShift(source: []u8, places: usize) void {
    if (source.len <= places or source[places] == 0) {
        source[0] = 0;
        return;
    }

    var i: usize = 0;
    while (i + places < source.len and source[i + places] != 0) : (i += 1) {
        source[i] = source[i + places];
    }
    source[i] = 0;
}

// Interface functions
pub fn shellRead() void {
    var position: usize = 0;
    @memset(&prompt, 0, MAX_CHAR_PROMPT);

    while (position < MAX_CHAR_PROMPT - 1) {
        const c = getchar();

        if (c == '\b') {
            if (position == 0) continue;
            putc(c);
            position -= 1;
            continue;
        } else if (c == '\n') {
            putc(c);
            prompt[position] = 0;
            return;
        } else {
            prompt[position] = c;
            putc(c);
        }

        position += 1;
    }

    putc('\n');
}

pub fn shellParse() void {
    var quote_flag = false;
    var quote_flag_state = QuoteFlagState.Free;

    @memset(&args, null, MAX_CMD_ARGS);
    argc = 0;
    promptPurify(&prompt);

    var begin: [*]u8 = &prompt;
    var end: [*]u8 = begin;

    while (end[0] != 0 and argc < MAX_CMD_ARGS - 1) {
        if (quote_flag_state == .Free) {
            switch (end[0]) {
                '"' => {
                    quote_flag = true;
                    quote_flag_state = .WaitForDoubleQuote;
                    promptShift(end[0..prompt.len], 1);
                },
                '\'' => {
                    quote_flag = true;
                    quote_flag_state = .WaitForSingleQuote;
                    promptShift(end[0..prompt.len], 1);
                },
                else => {},
            }
        } else if (quote_flag_state == .WaitForDoubleQuote) {
            if (end[0] == '"') {
                quote_flag = false;
                quote_flag_state = .Free;
                promptShift(end[0..prompt.len], 1);
            }
        } else if (end[0] == '\'') {
            quote_flag = false;
            quote_flag_state = .Free;
            promptShift(end[0..prompt.len], 1);
        }

        if (!quote_flag and (end[0] == ' ' or end[0] == '\t')) {
            end[0] = 0;
            args[@intCast(argc)] = begin;
            argc += 1;
            begin = end + 1;
        }

        end += 1;
    }

    args[@intCast(argc)] = begin;
    argc += 1;
}

pub fn shellExecute() void {
    const prompt_slice = prompt[0 .. std.mem.indexOf(u8, &prompt, &[_]u8{0}) orelse prompt.len];

    if (std.mem.eql(u8, prompt_slice, "help")) {
        helpCommand(argc, &args);
    } else if (std.mem.eql(u8, prompt_slice, "clear")) {
        VGA_clr();
    } else if (std.mem.eql(u8, prompt_slice, "exit")) {
        panic();
    } else if (std.mem.eql(u8, prompt_slice, "dumpsector")) {
        dumpsectorCommand(argc, &args);
    } else if (std.mem.eql(u8, prompt_slice, "usermode")) {
        usermodecommand(argc, &args);
    } else if (std.mem.eql(u8, prompt_slice, "physmeminfo")) {
        physmeminfoCommand(argc, &args);
    } else if (std.mem.eql(u8, prompt_slice, "readfile")) {
        readfileCommand(argc, &args);
    } else {
        printf("%s: Unknown command\n", .{prompt_slice.ptr});
    }

    puts("\n");
}

fn helpCommand(argc1: c_int, argv: [*]?[*]u8) void {
    _ = argc1;
    _ = argv;
    putc('\n');
    puts("  command");
    VGA_moveCursorTo(VGA_getCurrentLine(), 27);
    puts("description\n");
    puts("------------------------------------------------------------------------------\n");

    VGA_coloredPuts(" - help", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": display this message\n");

    VGA_coloredPuts(" - clear", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": clear the screen\n");

    VGA_coloredPuts(" - exit", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": halt the system (forever)\n");

    VGA_coloredPuts(" - dumpsector", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": read a sector on disk and display the content\n");

    VGA_coloredPuts(" - physmeminfo", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": physical memory manager information\n");

    VGA_coloredPuts(" - readfile", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": read a file from disk !\n");

    VGA_coloredPuts(" - usermode", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": jump and run a usermode program !\n");
}

fn physmeminfoCommand(argc: c_int, argv: [*]?[*]u8) void {
    var info: physmem_info_t = undefined;
    // PHYSMEM *************************
    // getMemoryInfo(&info);

    printf("bitmap size: %d\n", .{info.bitmapSize});
    printf("total block number: %d\n", .{info.totalBlockNumber});
    printf("total free block: %d\n", .{info.totalFreeBlock});
    printf("total used block: %d\n", .{info.totalUsedBlock});
}

fn usermodecommand(argc: c_int, argv: [*]?[*]u8) void {
    const fd1 = VFS_open("/userprog.bin", 0); // VFS_O_RDWR assumed as 0, adjust as needed

    if (fd1 < 0) {
        VGA_puts("failed to open the file\n");
        return;
    }

    VIRTMEM_mapPage(@ptrFromInt(0x80000000), false);
    _ = VFS_read(fd1, @ptrFromInt(0x80000000), 4095);
    VFS_close(fd1);

    switch_to_usermode(0x80000000 + 4095, 0x80000000);
}

fn readfileCommand(argc: c_int, argv: [*]?[*]u8) void {
    if (argc > 2 or argc < 2) {
        puts("Usage: readfile <path>");
        return;
    }

    var buffer: [10]u8 = undefined;
    const path = std.mem.span(argv[1].?);

    printf("opening %s\n", .{path.ptr});
    const fd1 = VFS_open(path, 0); // VFS_O_RDWR assumed as 0

    if (fd1 < 0) {
        VGA_puts("failed to open the file\n");
        return;
    }

    printf("content:\n");
    while (true) {
        const read = VFS_read(fd1, &buffer, 9);
        buffer[read] = 0;
        printf("%s", .{buffer.ptr});
        if (read == 0) break;
    }

    VFS_close(fd1);
}

// fn dumpsectorCommand(argc: c_int, argv: [*]?[*]u8) void {
//     var phys_buffer: [512]u8 = undefined;

//     if (argc > 2 or argc < 2) {
//         puts("Usage: dumpsector <sector number>");
//         return;
//     }

//     const sector = std.fmt.parseInt(u16, std.mem.span(argv[1].?), 0) catch 0;
//     FDC_readSectors(&phys_buffer, sector, 1);

//     for (0..2) |i| {
//         for (0..256) |j| {
//             printf("0x%x ", .{phys_buffer[j + 256 * i]});
//         }
//     }
//     std.heap.page_allocator;
// }
/// bo guarda vedi te
fn dumpsectorCommand(argc: c_int, argv: [*]?[*]u8) void {
    _ = argc;
    var phys_buffer: [512]u8 = undefined;

    if (argc > 2 or argc < 2) {
        puts("Usage: dumpsector <sector number>");
        return;
    }

    const sector = std.fmt.parseInt(u16, std.mem.span(argv[1].?), 0) catch 0;
    FDC_readSectors(&phys_buffer, sector, 1);

    for (0..2) |i| {
        for (0..256) |j| {
            printf("0x%x ", .{phys_buffer[j + 256 * i]});
        }
        puts("\n\rPress any key to continue\n\r");
        waitForKeyPress();
    }
}

pub fn shellUpdate() void {

    // console.
    std.heap.page_allocator;
}
