const std = @import("std");
const build_options = struct {
    isTest: bool = false,
};
pub fn build(b: *std.Build) void {
    var disabled_features = std.Target.Cpu.Feature.Set.empty;
    var enabled_features = std.Target.Cpu.Feature.Set.empty;

    disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.mmx));
    disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.sse));
    disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.sse2));
    disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.avx));
    disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.avx2));
    enabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.soft_float));

    const target_query = std.Target.Query{
        .cpu_arch = std.Target.Cpu.Arch.x86,
        .os_tag = std.Target.Os.Tag.freestanding,
        .abi = std.Target.Abi.none,
        .cpu_features_sub = disabled_features,
        .cpu_features_add = enabled_features,
    };
    const optimize = b.standardOptimizeOption(.{});
    const opts = b.addOptions();
    opts.addOption(enum { None, Initialisation, Scheduler, Panic }, "test_mode", .None);
    // exec_options.addOption(TestMode, "test_mode", test_mode);
    // const build_options = b.addOptions();
    // build_options.addOption(bool, "build_options", b.option("enable_debug", false));
    const target = b.resolveTargetQuery(target_query);
    const kernel = b.addExecutable(.{
        .name = "kernel.elf",
        .root_source_file = b.path("src/kmain.zig"),
        .target = target,
        .optimize = optimize,
        .code_model = .kernel,
    });

    kernel.root_module.addOptions("build_options", opts);

    // kernel.addOptions("build_options", unit_test_options);

    kernel.setLinkerScript(b.path("src/linker.ld"));
    b.installArtifact(kernel);

    const kernel_step = b.step("kernel", "Build the kernel");
    kernel_step.dependOn(&kernel.step);
}
