# qtvulkan

compiles, runs, but does not show anything yet.
requires setting VK_LAYER_PATH and LD_LIBRARY_PATH, e.g.

* LD_LIBRARY_PATH=$HOME/Vulkan-LoaderAndValidationLayers/loader
* VK_LAYER_PATH=$HOME/Vulkan-LoaderAndValidationLayers/dbuild/layers

or where that lies on your system.

The plan is to port the rotating cube demo, as well as have a Qt Widget that contains a lot of the boilerplate for vulkan setup.
