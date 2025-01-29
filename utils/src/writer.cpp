
#include "writer.h"

int create_directory(string &path) {
    mode_t mode = 0755;
    if (mkdir(path.c_str(), mode) == -1) {
        if (errno == EEXIST) {
            std::cerr << "Directory already exists: " << path << std::endl;
        } else {
            perror("Error creating directory");
            return -1;
        }
    } else {
        std::cout << "Directory created: " << path << std::endl;
    }
    return 0;
}