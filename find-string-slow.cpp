#include <cstdlib>
#include <iostream>
#include <cstring>

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

    const size_t BUFFER_SIZE = 8192;
    char substr[BUFFER_SIZE];
    size_t str_size = strlen(argv[1]);
    size_t p = 0;
    bool found = false;

    for (;;)
    {
        char buffer[BUFFER_SIZE];
        size_t bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file);

        if (bytes_read == 0)
        {
            if (ferror(file))
            {
                perror("fread failed");
                fclose(file);
                return EXIT_FAILURE;
            }
            break;
        }

        for (size_t i = 0; !found && i != bytes_read; ++i)
        {
            substr[p++] = buffer[i];
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

        if (found)
        {
            break;
        }
    }

    fclose(file);
    std::cout << (found ? "true\n" : "false\n");
    return EXIT_SUCCESS;
}

