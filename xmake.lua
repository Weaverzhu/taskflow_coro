add_rules("mode.debug", "mode.release")

add_requires("folly", "gtest", "libunwind", "benchmark", "taskflow")
set_languages("c++20")

target("test")
    set_kind("binary")
    add_files("taskflow_coro/*.cpp")
    add_includedirs(".")

    add_packages("folly", "gtest", "libunwind", "benchmark")


target("benchmark")
    set_kind("binary")
    add_files("benchmark/*.cpp")
    add_includedirs(".")
    add_packages("folly", "gtest", "libunwind", "benchmark", "taskflow")
