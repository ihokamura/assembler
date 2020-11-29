#include "generate.h"

/*
main function of assembler
*/
int main(int argc, char *argv[])
{
    const char *file_name = "test.o";
    generate(file_name);

    return 0;
}
