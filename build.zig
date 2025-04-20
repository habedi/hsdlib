const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{}); // Defaults to .Debug if not specified

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Prepare common C flags
    var common_c_flags = std.ArrayList([]const u8).init(allocator);
    defer common_c_flags.deinit();
    try common_c_flags.appendSlice(&.{ "-Wall", "-Wextra", "-pedantic", "-std=c11" });
    if (optimize == .Debug) {
        try common_c_flags.append("-DHSD_DEBUG=1");
    }

    // List of the C source files for the library
    const lib_source_files = &.{
        "src/utils.c",
        "src/distance/euclidean.c",
        "src/distance/hamming.c",
        "src/distance/manhattan.c",
        "src/similarity/cosine.c",
        "src/similarity/dot.c",
        "src/similarity/jaccard.c",
    };

    // --- Build Static Library ---
    const libhsd_static = b.addStaticLibrary(.{
        .name = "hsd",
        .target = target,
        .optimize = optimize,
    });

    libhsd_static.addIncludePath(b.path("include"));
    libhsd_static.addIncludePath(b.path("src"));
    libhsd_static.addCSourceFiles(.{
        .files = lib_source_files,
        .flags = common_c_flags.items,
    });

    libhsd_static.linkSystemLibrary("c");
    libhsd_static.linkSystemLibrary("m");

    b.installArtifact(libhsd_static);
    const static_step = b.step("static", "Build the static library");
    static_step.dependOn(&libhsd_static.step);

    // --- Build Shared Library ---
    const libhsd_shared = b.addSharedLibrary(.{
        .name = "hsd",
        .target = target,
        .optimize = optimize,
    });

    libhsd_shared.addIncludePath(b.path("include"));
    libhsd_shared.addIncludePath(b.path("src"));
    libhsd_shared.addCSourceFiles(.{
        .files = lib_source_files,
        .flags = common_c_flags.items,
    });

    libhsd_shared.linkSystemLibrary("c");
    libhsd_shared.linkSystemLibrary("m");

    b.installArtifact(libhsd_shared);
    const shared_step = b.step("shared", "Build the shared library");
    shared_step.dependOn(&libhsd_shared.step);

    const lib_step = b.step("lib", "Build static and shared libraries");
    lib_step.dependOn(static_step);
    lib_step.dependOn(shared_step);

    // --- Build C Test Runner ---
    const test_runner = b.addExecutable(.{
        .name = "test_runner",
        .target = target,
        .optimize = optimize,
    });

    test_runner.addIncludePath(b.path("include"));
    test_runner.addIncludePath(b.path("tests"));
    test_runner.addCSourceFiles(.{
        .files = &.{
            "tests/main.c",
            "tests/test_common.c",
            "tests/test_cosine.c",
            "tests/test_dot.c",
            "tests/test_euclidean.c",
            "tests/test_hamming.c",
            "tests/test_utils.c",
            "tests/test_jaccard.c",
            "tests/test_manhattan.c",
        },
        .flags = common_c_flags.items,
    });

    test_runner.linkLibrary(libhsd_static);
    test_runner.linkSystemLibrary("c");
    test_runner.linkSystemLibrary("m");

    const run_c_test_step = b.addRunArtifact(test_runner);
    run_c_test_step.step.dependOn(&test_runner.step);
    b.installArtifact(test_runner);

    const test_step = b.step("test-c", "Compile and run the C tests");
    test_step.dependOn(&run_c_test_step.step);
}
