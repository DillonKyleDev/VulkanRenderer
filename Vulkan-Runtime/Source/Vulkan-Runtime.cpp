#pragma once
#include "Vulkan-Core.h"

// standard library
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main()
{
	//VCore::SetupVulkan();

    bool _quit = false;
    VCore::VulkanManager app = VCore::VulkanManager();

    while (!_quit)
    {
        try {
            app.run(_quit);
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    return 1;
}