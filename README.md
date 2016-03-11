# MOD2C
> NMODL to C converter for CoreNEURON

MOD2C is NMODL to C converted for [CoreNEURON simulator](https://github.com/BlueBrain/CoreNEURON).
More information about NMODL can be found [here](https://www.neuron.yale.edu/neuron/static/docs/refman/nocmodl.html).

# Requirements
* [CMake 2.8.9+](https://cmake.org/)
* [Flex](http://flex.sourceforge.net)
* [Bison](https://www.gnu.org/software/bison/)

# Installation

Once you clone the repository, you can build mod2c using CMake as:

```bash
cd mod2c
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/isntall/directory
make
make install
```

Note that the mod2c is used only during the compilation phase of the CoreNEURON simulator and hence should be built only for the front-end/login nodes (in cross compile environments like BG-Q, Cray, Intel MIC etc.)

## License
* See LICENSE.txt
* See [NEURON](https://www.neuron.yale.edu/neuron/)