#!/bin/bash
set -ex

function check_style {
    export PATH=/usr/bin:$PATH
    #pre-commit install
    clang-format --version

    if ! pre-commit run -a ; then
        git diff
        exit 1
    fi
}

function cmake_ {
    cmake ..
}

function build {
    run_exe_test 1
    run_exe_test 2
    run_exe_test 3
    run_exe_test 4
    run_exe_test 5
    run_exe_test 6
    run_exe_test 7

    make -j8
}

function run_exe_test {
    export MALLOC_CHECK_=2
    local no=$1
    make exe_test$no -j8
    ctest -R exe_test$no -V
}

function test_ {
    ctest -V
}

function CI {
    mkdir -p build
    cd build
    cmake_
    build
    test_
}

function main {
    # Parse command line.
    for i in "$@"; do
        case $i in
            check_style)
                check_style
                shift
                ;;
            cmake)
                cmake_
                shift
                ;;
            build)
                build
                shift
                ;;
            test)
                test_
                shift
                ;;
            ci)
                CI
                shift
                ;;
        esac
    done
}


main $@
