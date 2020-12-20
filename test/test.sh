#!/bin/bash

# save the current directory
pushd ./test > /dev/null


# function to execute a test case
test()
{
    # set arguments
    source=$1
    expected=$2

    # assemble the source code
    asm=../asm
    object=${source%.*}.o
    $asm $source -c -o $object

    # link the object file with the standard library
    external=test_function.c
    binary=test_bin
    gcc $object $external -o $binary

    # run the binary and check the return value
    ./$binary
    actual=$?
    if [ $expected == $actual ]; then
        echo passed
    else
        echo $expected expected, but got $actual
    fi
}


# execute tests
test test.s 0


# restore the directory
popd > /dev/null
