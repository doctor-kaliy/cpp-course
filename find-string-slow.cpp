#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cstdio>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " <string> <filename>\n";
        return EXIT_FAILURE;
    }

    FILE* file = fopen(argv[2], "r");
    if (!file)
    {
        perror("fopen failed");
        return EXIT_FAILURE;
    }

    char substr[8192];
    size_t str_size = strlen(argv[1]);
    size_t p = 0;
    bool found = false;
    for (;;)
    {
        int c = getc(file);
        if (found || c == EOF)
        {
            if (ferror(file))
            {
                perror("fread failed");
                fclose(file);
                return EXIT_FAILURE;
            }
            break;
        }
        substr[p++] = (char) c;
        if (p == str_size)
        {
            found |= (strncmp(substr, argv[1], str_size) == 0);
            p--;
            for (size_t j = 0; j + 1 < str_size; ++j)
            {
                substr[j] = substr[j + 1];
            }
        }
    }

    fclose(file);
    fwrite((found ? "true\n" : "false\n"), sizeof(char), 6 - (size_t) found, stdout);
    return EXIT_SUCCESS;
}
