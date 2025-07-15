const std = @import("std");
const VFS_FD_DEBUG = 2; // Assuming 2 is the file descriptor for debug output// Simplified fputc for debug output (will need to be properly implemented later)
fn fputc(c: u8, file: u32) void {
    _ = file; // Suppress unused variable warning
    std.debug.print("{c}", .{c});
}
fn fputs(str: []const u8, file: u32) void {
    _ = file; // Suppress unused variable warning
    std.debug.print("{s}", .{str});
}
const PrintfState = enum {
    Normal,
    Length,
    LengthShort,
    LengthLong,
    Spec,
};
const PrintfLength = enum {
    Default,
    ShortShort,
    Short,
    Long,
    LongLong,
};
const g_HexChars = "0123456789abcdef";
fn fprintf_unsigned(file: u32, number: u64, radix: u32) void {
    var buffer: [32]u8 = undefined;
    var pos: usize = 0;
    var num = number;
    while (true) {
        const rem = num % radix;
        num /= radix;
        buffer[pos] = g_HexChars[rem];
        pos += 1;
        if (num == 0) break;
    }
    while (pos > 0) {
        pos -= 1;
        fputc(buffer[pos], file);
    }
}
fn fprintf_signed(file: u32, number: i64, radix: u32) void {
    if (number < 0) {
        fputc('-', file);
        fprintf_unsigned(file, @intCast(u64, -number), radix);
    } else {
        fprintf_unsigned(file, @intCast(u64, number), radix);
    }
}
pub fn vfprintf(file: u32, fmt: []const u8, args: anytype) void {
    var state = PrintfState.Normal;
    var length = PrintfLength.Default;
    var radix: u32 = 10;
    var sign = false;
    var number_flag = false;
    var fmt_it = std.mem.tokenizeAny(u8, fmt, "");
    while (fmt_it.next()) |char_slice| {
        const c = char_slice[0]; // Get the first character of the slice
        switch (state) {
            .Normal => {
                switch (c) {
                    '%' => state = .Length,
                    else => fputc(c, file),
                }
            },
            .Length => {
                switch (c) {
                    'h' => {
                        length = .Short;
                        state = .LengthShort;
                    },
                    'l' => {
                        length = .Long;
                        state = .LengthLong;
                    },
                    else => { // Fall through to Spec state
                        state = .Spec;
                        @fallthrough();
                    },
                }
            },
            .LengthShort => {
                if (c == 'h') {
                    length = .ShortShort;
                    state = .Spec;
                } else { // Fall through to Spec state
                    state = .Spec;
                    @fallthrough();
                }
            },
            .LengthLong => {
                if (c == 'l') {
                    length = .LongLong;
                    state = .Spec;
                } else { // Fall through to Spec state
                    state = .Spec;
                    @fallthrough();
                }
            },
            .Spec => {
                switch (c) {
                    'c' => {
                        fputc(args.next(u8) catch unreachable, file);
                    },
                    's' => {
                        fputs(args.next([]const u8) catch unreachable, file);
                    },
                    '%' => {
                        fputc('%', file);
                    },
                    'd', 'i' => {
                        radix = 10;
                        sign = true;
                        number_flag = true;
                    },
                    'u' => {
                        radix = 10;
                        sign = false;
                        number_flag = true;
                    },
                    'X', 'x', 'p' => {
                        radix = 16;
                        sign = false;
                        number_flag = true;
                    },
                    'o' => {
                        radix = 8;
                        sign = false;
                        number_flag = true;
                    },
                    else => {}, // Ignore invalid spec
                }
                if (number_flag) {
                    if (sign) {
                        switch (length) {
                            .ShortShort, .Short, .Default => {
                                fprintf_signed(file, args.next(i32) catch unreachable, radix);
                            },
                            .Long => {
                                fprintf_signed(file, args.next(i64) catch unreachable, radix);
                            },
                            .LongLong => {
                                fprintf_signed(file, args.next(i64) catch unreachable, radix);
                            },
                        }
                    } else {
                        switch (length) {
                            .ShortShort, .Short, .Default => {
                                fprintf_unsigned(file, args.next(u32) catch unreachable, radix);
                            },
                            .Long => {
                                fprintf_unsigned(file, args.next(u64) catch unreachable, radix);
                            },
                            .LongLong => {
                                fprintf_unsigned(file, args.next(u64) catch unreachable, radix);
                            },
                        }
                    }
                } // Reset state
                state = .Normal;
                length = .Default;
                radix = 10;
                sign = false;
                number_flag = false;
            },
        }
    }
}
pub fn debugc(c: u8) void {
    fputc(c, VFS_FD_DEBUG);
}
pub fn debugs(str: []const u8) void {
    fputs(str, VFS_FD_DEBUG);
}
pub fn debugf(comptime fmt: []const u8, args: anytype) void {
    vfprintf(VFS_FD_DEBUG, fmt, args);
}
pub fn debug_buffer(msg: []const u8, buffer: []const u8) void {
    debugs(msg);
    for (buffer_byte_ptr, i) |buffer_byte, _| {
        debugc(g_HexChars[buffer_byte >> 4]);
        debugc(g_HexChars[buffer_byte & 0xF]);
    }
    debugs("\n");
}
