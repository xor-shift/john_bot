def contains [str: string]: list -> bool {
    ($in | find $str | length) != 0
}

def select_option [sel_0: bool, opt_0_def: string, sel_1: bool, opt_1: string]: nothing -> string {
    let res = match [$sel_0, $sel_1] {
        [false, false] => $opt_0_def,
        [true, false] => $opt_0_def,
        [false, true] => $opt_1,
        [true, true] => {
            echo $"bad selection made, was expecting either ($opt_0_def) or ($opt_1)"
            exit 1
        }
    };

    $res
}

def main [...args: string] {
    let debug_build = $args | contains "debug";
    let release_build = $args | contains "release";

    let run_tests = $args | contains "test";
    let run_tsink = $args | contains "tsink";
    let do_cmake = $args | contains "cmake";

    let use_clang = $args | contains "clang";
    let use_gcc = $args | contains "gcc";

    let build_type = select_option $debug_build "debug" $release_build "release";
    echo $"build type: ($build_type)";
    let compiler_selection = select_option $use_gcc "gcc" $use_clang "clang";
    echo $"compiler selection: ($compiler_selection)";

    export-env { $env.CMAKE_COLOR_DIAGNOSTICS = "" };
    match $compiler_selection {
        "clang" => {
            export-env { $env.CC = "clang" };
            export-env { $env.CXX = "clang++" };
        },
        "gcc" => {
            export-env { $env.CC = "gcc" };
            export-env { $env.CXX = "g++" };
        },
        _ => { echo $"bad compiler selection: ($compiler_selection)"; exit 1; }
    };

    let run_type = if $run_tests { "test" } else if $run_tsink { "tsink" } else { "main" };
    echo $"run type: ($run_type)";

    let build_dir = $"build/($build_type)-($compiler_selection)";
    if ($build_dir | path exists) == false { mkdir $build_dir; }

    #### BUILD ####

    cd $build_dir;
    if $do_cmake {
         let cmake_build_type = match $build_type {
             "debug" => "Debug",
             "release" => "Release"
             _ => { echo $"bad build type: ($build_type)"; exit 1; }
         };
         cmake ../.. -G Ninja $"-DCMAKE_BUILD_TYPE=($cmake_build_type)";
    }
    ninja;
    cd ../..;
    cp $"($build_dir)/compile_commands.json" ./;

    #### RUN ####

    match $run_type {
        "main" => {
            if ("run/main" | path exists) == false { mkdir "run/main"; }
            cd run/main;
            run-external $"../../($build_dir)/john_bot";
            cd ..;
        },
        "test" => {
            if ("run/test" | path exists) == false { mkdir "run/test"; }
            cd run/test;
            run-external $"../../($build_dir)/john_bot_test";
            cd ..;
        },
        "tsink" => {
            run-external $"($build_dir)/terminal_sink";
        }
        _ => {
            echo "invalid executable name"
            return 1
        }
    }
}

