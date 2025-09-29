#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "core/application.h"

int main() {

    xe::Application app{};

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}