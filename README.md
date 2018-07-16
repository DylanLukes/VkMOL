# VkMOL

A proof-of-concept portable Vulkan-ized rendering render for molecular modeling software.

(**IMPORTANT: This project is in progress and not remotely complete.**)

## Getting Started

## Installing

> Note: Instructions for Windows are not currently available.
> It is almost certainly possible to use these instructions in Cygwin or the Linux Subsystem, but I have not tested it.

### CMake

You can obtain `cmake` from your package manager (`apt`, `yum`, `brew`, etc).

### Vulkan

Refer to the [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/).

Recent builds of `cmake` ship with a `FindVulkan.cmake` script which will find 
the Vulkan SDK.

Ensure you set the appropriate environment variables. On MacOS for example:

```bash
export VULKAN_SDK=/usr/local/opt/vulkan
export VK_ICD_FILENAMES=$VULKAN_SDK/etc/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=$VULKAN_SDK/etc/vulkan/explicit_layer.d
```

### CCP4

**Important: The pre-built CCP4 distributions will not work.**

The prebuilt CCP4 distributions available at `http://www.ccp4.ac.uk` are not
shipped with some headers we need (such as `clipper/clipper*.h`). 

You must download and build CCP4 from source. Follow the instructions in the 
`README`/`INSTALL` provided in the source tarball.

Finally, set the environment variable `CCP4` to your `ccp4-dev` directory.

## Running Tests

## Authors

* **Dylan Lukes**

See also the list of [contributors](https://github.com/DylanLukes/VkMOL/contributors) who participated in this project.

## License

This project is licensed under the MIT License – see the LICENSE file for details.

## Acknowledgements 

 - The users of the `##OpenGL` and `##vulkan` freenode channels.
 - [Turo Lamminen](https://github.com/turol/)
   - Resource management and general structure.
   - Heavy inspiration taken from: https://github.com/turol/smaaDemo