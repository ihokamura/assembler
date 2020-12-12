#include <stdio.h>
#include <string.h>

#include "generator.h"
#include "parser.h"
#include "tokenizer.h"

/*
main function of assembler
*/
int main(int argc, char *argv[])
{
    // parse arguments
    char *user_input = read_file(argv[1]);
    const char *output_file = argv[4];

    // tokenize input
    tokenize(user_input);

    // construct syntax tree
    Program program;
    construct(&program);

    // generate an object file
    generate(output_file, &program);

    return 0;
}
