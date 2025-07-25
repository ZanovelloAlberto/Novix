const std = @import("std");
const Allocator = std.mem.Allocator;
const testing = std.testing;
const log = std.log.scoped(.heap);
const builtin = std.builtin;
const is_test = builtin.is_test;
const build_options = @import("build_options");
const vmm = @import("vmm.zig");
const panic = @import("panic.zig").panic;

pub const FreeListAllocator = struct {
    const Error = error{TooSmall};
    const Header = struct {
        size: usize,
        next_free: ?*Header,

        const Self = @This();

        ///
        /// Initialise the header for a free allocation node
        ///
        /// Arguments:
        ///     IN size: usize - The node's size, not including the size of the header itself
        ///     IN next_free: ?*Header - A pointer to the next free node
        ///
        /// Return: Header
        ///     The header constructed
        fn init(size: usize, next_free: ?*Header) Header {
            return .{
                .size = size,
                .next_free = next_free,
            };
        }
    };
    const Self = @This();

    first_free: ?*Header,

    ///
    /// Initialise an empty and free FreeListAllocator
    ///
    /// Arguments:
    ///     IN start: usize - The starting address for all allocations
    ///     IN size: usize - The size of the region of memory to allocate within. Must be greater than @sizeOf(Header)
    ///
    /// Return: FreeListAllocator
    ///     The FreeListAllocator constructed
    ///
    /// Error: Error
    ///     Error.TooSmall - If size <= @sizeOf(Header)
    ///
    pub fn init(start: usize, size: usize) Error!FreeListAllocator {
        if (size <= @sizeOf(Header)) return Error.TooSmall;
        return FreeListAllocator{
            .first_free = insertFreeHeader(start, size - @sizeOf(Header), null),
        };
    }

    // const vtable = Allocator.VTable{
    //     .alloc = struct {
    //         fn f(ctx: *anyopaque, size: usize, alignment: u29, size_alignment: u29, ret_addr: usize) Allocator.Error![]u8 {
    //             const self = @ptrCast(*Self, @alignCast(ctx));
    //             return self.alloc(size, alignment, size_alignment, ret_addr);
    //         }
    //     }.f,
    //     .resize = struct {
    //         fn f(ctx: *anyopaque, old_mem: []u8, old_align: u29, new_size: usize, size_alignment: u29, ret_addr: usize) ?usize {
    //             const self = @ptrCast(*Self, @alignCast(ctx));
    //             return self.resize(old_mem, old_align, new_size, size_alignment, ret_addr);
    //         }
    //     }.f,
    //     .free = struct {
    //         fn f(ctx: *anyopaque, mem: []u8, alignment: u29, ret_addr: usize) void {
    //             const self = @ptrCast(*Self, @alignCast(ctx));
    //             self.free(mem, alignment, ret_addr);
    //         }
    //     }.f,
    // };

    const vtable = Allocator.VTable{
        .alloc = @ptrCast(&alloc),
        .resize = @ptrCast(&resize),
        .free = @ptrCast(&free),
        .remap = @ptrCast(&remap),
    };

    pub fn allocator(self: *Self) Allocator {
        return .{ .ptr = self, .vtable = &vtable };
    }

    ///
    /// Create a free header at a specific location
    ///
    /// Arguments:
    ///     IN at: usize - The address to create it at
    ///     IN size: usize - The node's size, excluding the size of the header itself
    ///     IN next_free: ?*Header - The next free header in the allocator, or null if there isn't one
    ///
    /// Return *Header
    ///     The pointer to the header created
    ///
    fn insertFreeHeader(at: usize, size: usize, next_free: ?*Header) *Header {
        const node: *Header = @ptrFromInt(at);
        node.* = Header.init(size, next_free);
        return node;
    }

    ///
    /// Update the free header pointers that should point to the provided header
    ///
    /// Arguments:
    ///     IN self: *FreeListAllocator - The FreeListAllocator to modify
    ///     IN previous: ?*Header - The previous free node or null if there wasn't one. If null, self.first_free will be set to header, else previous.next_free will be set to header
    ///     IN header: ?*Header - The header being pointed to. This will be the new value of self.first_free or previous.next_free
    ///
    fn registerFreeHeader(self: *Self, previous: ?*Header, header: ?*Header) void {
        if (previous) |p| {
            p.next_free = header;
        } else {
            self.first_free = header;
        }
    }

    ///
    /// Free an allocation
    ///
    /// Arguments:
    ///     IN self: *FreeListAllocator - The allocator being freed within
    ///     IN mem: []u8 - The memory to free
    ///     IN alignment: u29 - The alignment used to allocate the memory
    ///     IN ret_addr: usize - The return address passed by the high-level Allocator API. This is ignored.
    ///
    fn free(self: *Self, mem: []u8, alignment: u29, ret_addr: usize) !void {
        _ = alignment;
        _ = ret_addr;
        const size = @max(mem.len, @sizeOf(Header));
        const addr = @intFromPtr(mem.ptr);
        var header = insertFreeHeader(addr, size - @sizeOf(Header), null);
        if (self.first_free) |first| {
            var prev: ?*Header = null;
            // Find the previous free node
            if (@intFromPtr(first) < addr) {
                prev = first;
                while (prev.?.next_free) |next| {
                    if (@intFromPtr(next) > addr) break;
                    prev = next;
                }
            }
            // Make the freed header point to the next one, which is the one after the previous or the first if there was no previous
            header.next_free = if (prev) |p| p.next_free else first;

            self.registerFreeHeader(prev, header);

            // Join with the next one until the next isn't a neighbour
            if (header.next_free) |next| {
                if (@intFromPtr(next) == @intFromPtr(header) + header.size + @sizeOf(Header)) {
                    header.size += next.size + @sizeOf(Header);
                    header.next_free = next.next_free;
                }
            }

            // Try joining with the previous one
            if (prev) |p| {
                p.size += header.size + @sizeOf(Header);
                p.next_free = header.next_free;
            }
        } else {
            self.first_free = header;
        }
    }

    pub fn remap(
        ctx: *anyopaque,
        mem: []u8,
        alignment: std.mem.Alignment,
        new_len: usize,
        ret_addr: usize,
    ) Allocator.Error![]u8 {
        // Remap (realloc) implementation for FreeListAllocator
        // Try to resize in place, otherwise allocate new and copy
        // const self: *Self = @ptrCast(@alignCast(ctx));
        // if (new_len == mem.len) return mem[0..new_len];

        // const old_align: u29 = @intCast(alignment);
        // const size_alignment: u29 = @intCast(alignment);

        // const resized = self.resize(mem, old_align, new_len, size_alignment, ret_addr);
        // if (resized) |new_size| {
        //     if (new_size == 0) return null;
        //     return mem[0..new_size];
        // }

        // // Could not resize in place, allocate new and copy
        // const new_mem = self.alloc(new_len, old_align, size_alignment, ret_addr) catch return null;
        // @memcpy(new_mem, mem);
        // self.free(mem, old_align, ret_addr);
        // return new_mem[0..new_mem.len];
        _ = ctx;
        _ = mem;
        _ = alignment;
        _ = new_len;
        _ = ret_addr;
        return &[_]u8{};
    }

    ///
    /// Attempt to resize an allocation. This should only be called via the Allocator interface.
    ///
    /// When the new size requested is 0, a free happens. See the free function for details.
    ///
    /// When the new size is greater than the old buffer's size, we attempt to steal some space from the neighbouring node.
    /// This can only be done if the neighbouring node is free and the remaining space after taking what is needed to resize is enough to create a new Header. This is because we don't want to leave any dangling memory that isn't tracked by a header.
    ///
    /// | <----- new_size ----->
    /// |---------|--------\----------------|
    /// |         |        \                |
    /// | old_mem | header \ header's space |
    /// |         |        \                |
    /// |---------|--------\----------------|
    ///
    /// After expanding to new_size, it will look like
    /// |-----------------------|--------\--|
    /// |                       |        \  |
    /// |        old_mem        | header \  |
    /// |                       |        \  |
    /// |-----------------------|--------\--|
    /// The free node before old_mem needs to then point to the new header rather than the old one and the new header needs to point to the free node after the old one. If there was no previous free node then the new one becomes the first free node.
    ///
    /// When the new size is smaller than the old_buffer's size, we attempt to shrink it and create a new header to the right.
    /// This can only be done if the space left by the shrinking is enough to create a new header, since we don't want to leave any dangling untracked memory.
    /// | <--- new_size --->
    /// |-----------------------------------|
    /// |                                   |
    /// |             old_mem               |
    /// |                                   |
    /// |-----------------------------------|
    ///
    /// After shrinking to new_size, it will look like
    /// | <--- new_size --->
    /// |-------------------|--------\-- ---|
    /// |                   |        \      |
    /// |      old_mem      | header \      |
    /// |                   |        \      |
    /// |-------------------|--------\------|
    /// We then attempt to join with neighbouring free nodes.
    /// The node before old_mem needs to then point to the new header and the new header needs to point to the next free node.
    ///
    /// Arguments:
    ///     IN self: *FreeListAllocator - The allocator to resize within.
    ///     IN old_mem: []u8 - The buffer to resize.
    ///     IN old_align: u29 - The original alignment for old_mem.
    ///     IN new_size: usize - What to resize to.
    ///     IN size_alignment: u29 - The alignment that the size should have.
    ///     IN ret_addr: usize - The return address passed by the high-level Allocator API. This is ignored
    ///
    /// Return: ?usize
    ///     The new size of the buffer, which will be new_size if the operation was successfull, or null if the operation wasn't successful.
    ///
    fn resize(self: *Self, old_mem: []u8, old_align: u29, new_size: usize, size_alignment: u29, ret_addr: usize) bool {
        // Suppress unused var warning

        if (new_size == 0) {
            self.free(old_mem, old_align, ret_addr) catch false;
            return true;
        }
        if (new_size == old_mem.len) return true;

        const end = @intFromPtr(old_mem.ptr) + old_mem.len;
        var real_size = if (size_alignment > 1) std.mem.alignAllocLen(old_mem.len, new_size, size_alignment) else new_size;

        // Try to find the buffer's neighbour (if it's free) and the previous free node
        // We'll be stealing some of the free neighbour's space when expanding or joining up with it when shrinking
        var free_node = self.first_free;
        var next: ?*Header = null;
        var prev: ?*Header = null;
        while (free_node) |f| {
            if (@intFromPtr(f) == end) {
                // This free node is right next to the node being freed so is its neighbour
                next = f;
                break;
            } else if (@intFromPtr(f) > end) {
                // We've found a node past the node being freed so end early
                break;
            }
            prev = f;
            free_node = f.next_free;
        }

        // If we're expanding the buffer
        if (real_size > old_mem.len) {
            if (next) |n| {
                // If the free neighbour isn't big enough then fail
                if (old_mem.len + n.size + @sizeOf(Header) < real_size) return false;

                const size_diff = real_size - old_mem.len;
                const consumes_whole_neighbour = size_diff == n.size + @sizeOf(Header);
                // If the space left over in the free neighbour from the resize isn't enough to fit a new node, then fail
                if (!consumes_whole_neighbour and n.size + @sizeOf(Header) - size_diff < @sizeOf(Header)) return false;
                var new_next: ?*Header = n.next_free;
                // We don't do any splitting when consuming the whole neighbour
                if (!consumes_whole_neighbour) {
                    // Create the new header. It starts at the end of the buffer plus the stolen space
                    // The size will be the previous size minus what we stole
                    new_next = insertFreeHeader(end + size_diff, n.size - size_diff, n.next_free);
                }
                self.registerFreeHeader(prev, new_next);
                return true;
            }
            // The neighbour isn't free so we can't expand into it
            return false;
        } else {
            // Shrinking
            const size_diff = old_mem.len - real_size;
            // If shrinking would leave less space than required for a new header,
            // or if shrinking would make the buffer too small, don't shrink
            if (size_diff < @sizeOf(Header)) {
                return false;
            }
            // Make sure the we have enough space for a header
            if (real_size < @sizeOf(Header)) {
                real_size = @sizeOf(Header);
            }

            // Create a new header for the space gained from shrinking
            var new_next = insertFreeHeader(@intFromPtr(old_mem.ptr) + real_size, size_diff - @sizeOf(Header), if (prev) |p| p.next_free else self.first_free);
            self.registerFreeHeader(prev, new_next);

            // Join with the neighbour
            if (next) |n| {
                new_next.size += n.size + @sizeOf(Header);
                new_next.next_free = n.next_free;
            }

            return true;
        }
    }

    ///
    /// Allocate a portion of memory. This should only be called via the Allocator interface.
    ///
    /// This will find the first free node within the heap that can fit the size requested. If the size of the node is larger than the requested size but any space left over isn't enough to create a new Header, the next node is tried. If the node would require some padding to reach the desired alignment and that padding wouldn't fit a new Header, the next node is tried (however this node is kept as a backup in case no future nodes can fit the request).
    ///
    /// |--------------\---------------------|
    /// |              \                     |
    /// | free header  \     free space      |
    /// |              \                     |
    /// |--------------\---------------------|
    ///
    /// When the alignment padding is large enough for a new Header, the node found is split on the left, like so
    /// <---- padding ---->
    /// |------------\-----|-------------\---|
    /// |            \     |             \   |
    /// | new header \     | free header \   |
    /// |            \     |             \   |
    /// |------------\-----|-------------\---|
    /// The previous free node should then point to the left split. The left split should point to the free node after the one that was found
    ///
    /// When the space left over in the free node is more than required for the allocation, it is split on the right
    /// |--------------\-------|------------\--|
    /// |              \       |            \  |
    /// | free header  \ space | new header \  |
    /// |              \       |            \  |
    /// |--------------\-------|------------\--|
    /// The previous free node should then point to the new node on the left and the new node should point to the next free node
    ///
    /// Splitting on the left and right can both happen in one allocation
    ///
    /// Arguments:
    ///     IN self: *FreeListAllocator - The allocator to use
    ///     IN size: usize - The amount of memory requested
    ///     IN alignment: u29 - The alignment that the address of the allocated memory should have
    ///     IN size_alignment: u29 - The alignment that the length of the allocated memory should have
    ///     IN ret_addr: usize - The return address passed by the high-level Allocator API. This is ignored
    ///
    /// Return: []u8
    ///     The allocated memory
    ///
    /// Error: std.Allocator.Error
    ///     std.Allocator.Error.OutOfMemory - There wasn't enough memory left to fulfill the request
    ///
    pub fn alloc(self: *Self, size: usize, alignment: std.mem.Alignment, ret_addr: usize) ?[*]u8 {
        // Suppress unused var warning
        _ = ret_addr;
        _ = alignment;
        if (size == 0) return null;

        // The size must be at least the size of a header so that it can be freed properly
        const real_size = @max(size, @sizeOf(Header));

        // Search for a free block
        var prev: ?*Header = null;
        var header = self.first_free;
        while (header) |h| {
            if (h.size >= real_size) {
                // Remove from free list
                self.registerFreeHeader(prev, h.next_free);

                // If the block is big enough to split, do so
                if ((h.size - real_size) >= (@sizeOf(Header) + 16)) {
                    const new_header_addr = @intFromPtr(h) + @sizeOf(Header) + real_size;
                    const new_header = insertFreeHeader(new_header_addr, h.size - real_size - @sizeOf(Header), h.next_free);
                    self.registerFreeHeader(prev, new_header);
                    h.size = real_size;
                }

                // Return pointer to usable memory (after header)
                return @as([*]u8, @ptrFromInt(@intFromPtr(h) + @sizeOf(Header)));
            }
            prev = h;
            header = h.next_free;
        }

        // No suitable free block found
        return null;
    }

    test "init" {
        const size = 1024;
        const region = try testing.allocator.alloc(u8, size);
        defer testing.allocator.free(region);
        const free_list = &(try FreeListAllocator.init(@intFromPtr(region.ptr), size));

        const header: *FreeListAllocator.Header = @ptrFromInt(@intFromPtr(region.ptr));
        try testing.expectEqual(header, free_list.first_free.?);
        try testing.expectEqual(header.next_free, null);
        try testing.expectEqual(header.size, size - @sizeOf(Header));

        try testing.expectError(Error.TooSmall, FreeListAllocator.init(0, @sizeOf(Header) - 1));
    }

    test "alloc" {
        const size = 1024;
        const region = try testing.allocator.alloc(u8, size);
        defer testing.allocator.free(region);
        const start = @intFromPtr(region.ptr);
        var free_list = &(try FreeListAllocator.init(start, size));

        std.debug.print("", .{});

        const alloc0 = try free_list.alloc(64, 0, 0, @returnAddress());
        const alloc0_addr = @intFromPtr(alloc0.ptr);
        // Should be at the start of the heap
        try testing.expectEqual(alloc0_addr, start);
        // The allocation should have produced a node on the right of the allocation
        var header: *Header = @ptrFromInt(start + 64);
        try testing.expectEqual(header.size, size - 64 - @sizeOf(Header));
        try testing.expectEqual(header.next_free, null);
        try testing.expectEqual(free_list.first_free, header);

        std.debug.print("", .{});

        // 64 bytes aligned to 4 bytes
        const alloc1 = try free_list.alloc(64, 4, 0, @returnAddress());
        const alloc1_addr = @intFromPtr(alloc1.ptr);
        const alloc1_end = alloc1_addr + alloc1.len;
        // Should be to the right of the first allocation, with some alignment padding in between
        const alloc0_end = alloc0_addr + alloc0.len;
        try testing.expect(alloc0_end <= alloc1_addr);
        try testing.expectEqual(std.mem.alignForward(alloc0_end, 4), alloc1_addr);
        // It should have produced a node on the right
        header = @ptrFromInt(alloc1_end);
        try testing.expectEqual(header.size, size - (alloc1_end - start) - @sizeOf(Header));
        try testing.expectEqual(header.next_free, null);
        try testing.expectEqual(free_list.first_free, header);

        const alloc2 = try free_list.alloc(64, 256, 0, @returnAddress());
        const alloc2_addr = @intFromPtr(alloc2.ptr);
        const alloc2_end = alloc2_addr + alloc2.len;
        try testing.expect(alloc1_end < alloc2_addr);
        // There should be a free node to the right of alloc2
        const second_header: *Header = @ptrFromInt(alloc2_end);
        try testing.expectEqual(second_header.size, size - (alloc2_end - start) - @sizeOf(Header));
        try testing.expectEqual(second_header.next_free, null);
        // There should be a free node in between alloc1 and alloc2 due to the large alignment padding (depends on the allocation by the testing allocator, hence the check)
        if (alloc2_addr - alloc1_end >= @sizeOf(Header)) {
            header = @ptrFromInt(alloc1_end);
            try testing.expectEqual(free_list.first_free, header);
            try testing.expectEqual(header.next_free, second_header);
        }

        // Try allocating something smaller than @sizeOf(Header). This should scale up to @sizeOf(Header)
        const alloc3 = try free_list.alloc(1, 0, 0, @returnAddress());
        const alloc3_addr = @intFromPtr(alloc3.ptr);
        const alloc3_end = alloc3_addr + @sizeOf(Header);
        const header2: *Header = @ptrFromInt(alloc3_end);
        // The new free node on the right should be the first one free
        try testing.expectEqual(free_list.first_free, header2);
        // And it should point to the free node on the right of alloc2
        try testing.expectEqual(header2.next_free, second_header);

        // Attempting to allocate more than the size of the largest free node should fail
        const remaining_size = second_header.size + @sizeOf(Header);
        try testing.expectError(Allocator.Error.OutOfMemory, free_list.alloc(remaining_size + 1, 0, 0, @returnAddress()));

        // Alloc a non aligned to header
        const alloc4 = try free_list.alloc(13, 1, 0, @returnAddress());
        const alloc4_addr = @intFromPtr(alloc4.ptr);
        const alloc4_end = alloc4_addr + std.mem.alignForward(13, @alignOf(Header));
        const header3: *Header = @ptrFromInt(alloc4_end);

        // We should still have a length of 13
        try testing.expectEqual(alloc4.len, 13);
        // But this should be aligned to Header (4)
        try testing.expectEqual(alloc4_end - alloc4_addr, 16);

        // Previous header should now point to the next header
        try testing.expectEqual(header2.next_free, header3);
    }

    test "free" {
        const size = 1024;
        const region = try testing.allocator.alloc(u8, size);
        defer testing.allocator.free(region);
        const start = @intFromPtr(region.ptr);
        var free_list = &(try FreeListAllocator.init(start, size));

        const alloc0 = try free_list.alloc(128, 0, 0, @returnAddress());
        const alloc1 = try free_list.alloc(256, 0, 0, @returnAddress());
        const alloc2 = try free_list.alloc(64, 0, 0, @returnAddress());

        // There should be a single free node after alloc2
        const free_node3: *Header = @ptrFromInt(@intFromPtr(alloc2.ptr) + alloc2.len);
        try testing.expectEqual(free_list.first_free, free_node3);
        try testing.expectEqual(free_node3.size, size - alloc0.len - alloc1.len - alloc2.len - @sizeOf(Header));
        try testing.expectEqual(free_node3.next_free, null);

        free_list.free(alloc0, 0, 0);
        // There should now be two free nodes. One where alloc0 was and another after alloc2
        const free_node0: *Header = @ptrFromInt(start);
        try testing.expectEqual(free_list.first_free, free_node0);
        try testing.expectEqual(free_node0.size, alloc0.len - @sizeOf(Header));
        try testing.expectEqual(free_node0.next_free, free_node3);

        // Freeing alloc1 should join it with free_node0
        free_list.free(alloc1, 0, 0);
        try testing.expectEqual(free_list.first_free, free_node0);
        try testing.expectEqual(free_node0.size, alloc0.len - @sizeOf(Header) + alloc1.len);
        try testing.expectEqual(free_node0.next_free, free_node3);

        // Freeing alloc2 should then join them all together into one big free node
        free_list.free(alloc2, 0, 0);
        try testing.expectEqual(free_list.first_free, free_node0);
        try testing.expectEqual(free_node0.size, size - @sizeOf(Header));
        try testing.expectEqual(free_node0.next_free, null);
    }

    // test "resize" {
    //     std.debug.print("", .{});
    //     const size = 1024;
    //     var region = try testing.allocator.alloc(u8, size);
    //     defer testing.allocator.free(region);
    //     const start = @intFromPtr(region.ptr);
    //     var free_list = &(try FreeListAllocator.init(start, size));

    //     var alloc0 = try free_list.alloc(128, 0, 0, @returnAddress());
    //     var alloc1 = try free_list.alloc(256, 0, 0, @returnAddress());

    //     // Expanding alloc0 should fail as alloc1 is right next to it
    //     try testing.expectEqual(free_list.resize(alloc0, 0, 136, 0, @returnAddress()), null);

    //     // Expanding alloc1 should succeed
    //     try testing.expectEqual(free_list.resize(alloc1, 0, 512, 0, @returnAddress()), 512);
    //     alloc1 = alloc1.ptr[0..512];
    //     // And there should be a free node on the right of it
    //     var header: *Header = @ptrFromInt(@intFromPtr(alloc1.ptr) + 512);
    //     try testing.expectEqual(header.size, size - 128 - 512 - @sizeOf(Header));
    //     try testing.expectEqual(header.next_free, null);
    //     try testing.expectEqual(free_list.first_free, header);

    //     // Shrinking alloc1 should produce a big free node on the right
    //     try testing.expectEqual(free_list.resize(alloc1, 0, 128, 0, @returnAddress()), 128);
    //     alloc1 = alloc1.ptr[0..128];
    //     header = @ptrFromInt(@intFromPtr(alloc1.ptr) + 128);
    //     try testing.expectEqual(header.size, size - 128 - 128 - @sizeOf(Header));
    //     try testing.expectEqual(header.next_free, null);
    //     try testing.expectEqual(free_list.first_free, header);

    //     // Shrinking by less space than would allow for a new Header shouldn't work
    //     try testing.expectEqual(free_list.resize(alloc1, 0, alloc1.len - @sizeOf(Header) / 2, 0, @returnAddress()), 128);
    //     // Shrinking to less space than would allow for a new Header shouldn't work
    //     try testing.expectEqual(free_list.resize(alloc1, 0, @sizeOf(Header) / 2, 0, @returnAddress()), @sizeOf(Header));
    // }
};

///
/// Initialise the kernel heap with a chosen allocator
///
/// Arguments:
///     IN vmm_payload: type - The payload passed around by the VMM. Decided by the architecture
///     IN heap_vmm: *vmm.VirtualMemoryManager - The VMM associated with the kernel
///     IN attributes: vmm.Attributes - The attributes to associate with the memory allocated for the heap
///     IN heap_size: usize - The desired size of the heap, in bytes. Must be greater than @sizeOf(FreeListAllocator.Header)
///
/// Return: FreeListAllocator
///     The FreeListAllocator created to keep track of the kernel heap
///
/// Error: FreeListAllocator.Error || Allocator.Error
///     FreeListAllocator.Error.TooSmall - heap_size is too small
///     Allocator.Error.OutOfMemory - heap_vmm's allocator didn't have enough memory available to fulfill the request
///
pub fn init(comptime vmm_payload: type, heap_vmm: *vmm.VirtualMemoryManager(vmm_payload), attributes: vmm.Attributes, heap_size: usize) (FreeListAllocator.Error || Allocator.Error)!FreeListAllocator {
    log.info("Init\n", .{});
    defer log.info("Done\n", .{});
    const heap_start = (try heap_vmm.alloc(heap_size / vmm.BLOCK_SIZE, null, attributes)) orelse panic(null, "Not enough contiguous virtual memory blocks to allocate to kernel heap\n", .{});
    // This free call cannot error as it is guaranteed to have been allocated above
    errdefer heap_vmm.free(heap_start) catch unreachable;
    return try FreeListAllocator.init(heap_start, heap_size);
}
