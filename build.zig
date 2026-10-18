const std = @import("std");

pub fn build(b: *std.Build) void {
    // Track optional directory removal and restoration in the configure cache.
    b.dependOnDirectoryContents(b.path("."));
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const c_flags: []const []const u8 = &.{ "-std=c11", "-Wall", "-Wextra", "-pedantic" };
    const executable_flags: []const []const u8 = &.{ "-std=gnu2x", "-Wall", "-Wextra", "-pedantic" };
    const sources: []const []const u8 = &.{
        "src/distance/euclidean.c",
        "src/distance/manhattan.c",
        "src/distance/hamming.c",
        "src/similarity/dot.c",
        "src/similarity/cosine.c",
        "src/similarity/jaccard.c",
        "src/utils.c",
    };

    var static_library: *std.Build.Step.Compile = undefined;
    for ([_]std.builtin.LinkMode{ .static, .dynamic }) |linkage| {
        const module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .pic = true,
        });
        module.addCSourceFiles(.{ .files = sources, .flags = c_flags });
        module.addIncludePath(b.path("include"));
        if (optimize == .debug) module.addCMacro("HSD_DEBUG", "1");
        if (target.result.os.tag != .windows) module.linkSystemLibrary("m", .{});
        const library = b.addLibrary(.{
            .name = "hsd",
            .linkage = linkage,
            .root_module = module,
        });
        b.installArtifact(library);
        if (linkage == .static) {
            static_library = library;
            library.installHeader(b.path("include/hsdlib.h"), "hsdlib.h");
        }
    }

    if (directoryExists(b, "tests")) {
        const tests_module = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
        tests_module.addIncludePath(b.path("tests"));
        tests_module.addCSourceFiles(.{
            .files = &.{
                "tests/main.c",           "tests/test_common.c",    "tests/test_utils.c",
                "tests/test_euclidean.c", "tests/test_manhattan.c", "tests/test_hamming.c",
                "tests/test_dot.c",       "tests/test_cosine.c",    "tests/test_jaccard.c",
            },
            .flags = executable_flags,
        });
        tests_module.linkLibrary(static_library);
        const tests = b.addExecutable(.{ .name = "test_runner", .root_module = tests_module });
        b.step("test", "Build and run C tests").dependOn(&b.addRunArtifact(tests).step);
    }

    if (directoryExists(b, "examples")) {
        const example_module = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
        example_module.addCSourceFile(.{
            .file = b.path("examples/hsdlib_example.c"),
            .flags = executable_flags,
        });
        example_module.linkLibrary(static_library);
        const example = b.addExecutable(.{ .name = "hsdlib_example", .root_module = example_module });
        b.step("example", "Build and run the C example").dependOn(&b.addRunArtifact(example).step);
    }
}

fn directoryExists(b: *std.Build, path: []const u8) bool {
    const directory = b.root.openDir(b.graph.io, path, .{}) catch |err| switch (err) {
        error.FileNotFound => return false,
        else => std.debug.panic("Unable to access {s}: {t}", .{ path, err }),
    };
    directory.close(b.graph.io);
    return true;
}
