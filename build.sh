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
    make -j8
}

function test_ {
    ctest
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
