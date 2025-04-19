// build.zig
const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // --- Build Static Library ---
    const libhsd_static = b.addStaticLibrary(.{
        .name = "hsd",
        .target = target,
        .optimize = optimize,
    });

    libhsd_static.addIncludePath(.{ .path = "include" });
    libhsd_static.addIncludePath(.{ .path = "src" }); // Allow internal includes if any
    libhsd_static.addCSourceFiles(.{
        .files = &.{
            "src/utils.c",
            "src/distance/euclidean.c",
            "src/distance/hamming.c",
            "src/distance/manhattan.c",
            "src/similarity/cosine.c",
            "src/similarity/dot.c",
            "src/similarity/jaccard.c",
        },
        // Pass CFLAGS - these are added to Zig's defaults based on target/optimize
        .flags = &.{ "-Wall", "-Wextra", "-pedantic", "-std=c11" },
    });

    libhsd_static.linkSystemLibrary("c"); // Often implicit, but good practice
    libhsd_static.linkSystemLibrary("m"); // Link math library

    if (optimize == .Debug) {
        libhsd_static.defineCMacro("HSD_DEBUG", "1");
    }

    b.installArtifact(libhsd_static);
    const static_step = b.step("static", "Build the static library");
    static_step.dependOn(&libhsd_static.step);

    // --- Build Shared Library ---
    const libhsd_shared = b.addSharedLibrary(.{
        .name = "hsd",
        .target = target,
        .optimize = optimize,
        // .version = .{ .major = 0, .minor = 1, .patch = 0 }, // Optional versioning
    });

    libhsd_shared.addIncludePath(.{ .path = "include" });
    libhsd_shared.addIncludePath(.{ .path = "src" });
    libhsd_shared.addCSourceFiles(.{
        .files = &.{
            "src/utils.c",
            "src/distance/euclidean.c",
            "src/distance/hamming.c",
            "src/distance/manhattan.c",
            "src/similarity/cosine.c",
            "src/similarity/dot.c",
            "src/similarity/jaccard.c",
        },
        .flags = &.{ "-Wall", "-Wextra", "-pedantic", "-std=c11" },
    });

    libhsd_shared.linkSystemLibrary("c");
    libhsd_shared.linkSystemLibrary("m");

    if (optimize == .Debug) {
        libhsd_shared.defineCMacro("HSD_DEBUG", "1");
    }

    b.installArtifact(libhsd_shared);
    const shared_step = b.step("shared", "Build the shared library");
    shared_step.dependOn(&libhsd_shared.step);

    // --- Build Default Lib Step (depends on both) ---
    const lib_step = b.step("lib", "Build static and shared libraries");
    lib_step.dependOn(static_step);
    lib_step.dependOn(shared_step);

    // --- Build C Test Runner ---
    const test_runner = b.addExecutable(.{
        .name = "test_runner",
        .target = target,
        .optimize = optimize,
    });

    test_runner.addIncludePath(.{ .path = "include" });
    test_runner.addIncludePath(.{ .path = "tests" });
    test_runner.addCSourceFiles(.{
        .files = &.{ // List all test sources
            "tests/main.c",
            "tests/test_common.c",
            "tests/test_cosine.c",
            "tests/test_dot.c",
            "tests/test_euclidean.c",
            "tests/test_hamming.c",
            "tests/test_hsdlib_h.c",
            "tests/test_jaccard.c",
            "tests/test_manhattan.c",
        },
        .flags = &.{ "-Wall", "-Wextra", "-pedantic", "-std=c11" },
    });

    // Link against the static library we built
    test_runner.linkLibrary(libhsd_static);
    test_runner.linkSystemLibrary("c");
    test_runner.linkSystemLibrary("m");

    if (optimize == .Debug) {
        test_runner.defineCMacro("HSD_DEBUG", "1");
    }

    // --- Define Test Step ---
    const run_c_test_step = b.addRunArtifact(test_runner);
    run_c_test_step.step.dependOn(b.getInstallStep()); // Ensure lib is installed first if runner needs it dynamically

    const test_step = b.step("test-c", "Compile and run the C tests");
    test_step.dependOn(&run_c_test_step.step);

}
