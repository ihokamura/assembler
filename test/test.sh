#!/bin/bash

ASM=$1
POSTFIX=$2

# save the current directory
pushd ./test > /dev/null


# function to execute a test case
test()
{
    # set arguments
    source=$1
    expected=$2

    # assemble the source code
    object=${source%.*}_${POSTFIX}.o
    $ASM $source -c -o $object

    # link the object file with the standard library
    external='test_utility.c external_text.c external_data.c'
    binary=${source%.*}_${POSTFIX}_bin
    gcc $object $external -o $binary

    # run the binary and check the return value
    echo $binary...
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
test test_mov.s 0
test test_nop.s 0


# restore the directory
popd > /dev/null
