<h1 align="center">MBPlib</h1>

MBPlib is a modular library for the development and trace-based simulation of branch predictors.

## Index

- [Design Goals](#design-goals)
- [Modules](#modules)
- [Usage](#usage)
  - [Obtaining Traces](#obtaining-traces)
- [Documentation](#documentation)
  - [Predictor Interface](#predictor-interface)
  - [Calling The Simulator](#calling-the-simulator)
  - [Completing the Output](#completing-the-output)

## Design Goals

MBPlib was designed with the following objectives in mind.

- **Simulation Speed**. A faster simulation speed allows testing many more configurations to find the optimal set of parameters and makes development in real-time possible, without having to wait for a long period until the simulations are finished. MBPlib is a thousand times faster than a cycle-accurate microarchitecture simulator like [ChampSim], because MBPlib only cares about the branches present in a program.

- **Development Speed**. We use MBPlib ourselves to research on branch prediction. Thus, we try to handle the most common usecases and offer utilities shared among branch predictors designs to avoid code duplication.

- **Interoperability**. MBPlib is built as a library instead of a framework. This means that you, as the user, will call MBPlib routines, instead of the other way around. Thus, you can use MBPlib along with any other library. For example, a genetics algorithm or a simulated annealing process. Furthermore, MBPlib's output is in JSON: no need to write a bunch of regular expressions to parse our output.

[ChampSim]: https://github.com/ChampSim/ChampSim

## Modules

MBPlib offers three modules.

- The **simulation library** is what would correspond to the whole framework in other software suites. Basically, it offers all the routines needed to run a user-defined branch predictor for a program trace and obtain a JSON object with the simulation results. The simulation library also offers the trace reader as a subcomponent for creating custom applications that inspect the program traces.

- The **utilities library** offers software implementations of components that are present in most branch predictors, such as fixed-width saturated counters or a class that maintains a hash of the global branch history. These components avoid the need to reimplement common functionality in different predictors. (You may also use this library to implement predictors for other simulators.)

- The **examples library** offers implementations for a lot of well-known branch predictors, including state-of-the-art predictors like BATAGE. These implementations can be used to learn and teach about branch prediction techniques and are useful examples of how to take advantage of the utilities library. In addition, you can also use some of these predictors as a subcomponent of a bigger predictor.

## Usage

[CMake]: https://cmake.org/
[example]: /example

MBPlib is built using [CMake]. The easiest way to add MBPlib as a dependency to your project is to clone the repository use the CMake command `include_subdirectory`. This is the way we use in our [example] folder, which [defines an executable](/example/CMakeLists.txt) for each predictor in the examples library.

You can start by compiling the examples. From the repository folder, type
```sh
cd example
cmake -S . -B build # or mkdir build; cd build; cmake ..; cd ..;
                    # if you have an old version of cmake.
cmake --build build
```
After that the build folder will contain a bunch of executables that can be run with the following command line options.
```sh
./build/<predictor> <trace> [<warmup instructions>] [<simulation instructions>]
```
For example, if you execute
```sh
./build/gshare_64KB traces/SHORT_SERVER-1.sbbt.zst
```
you will get the following output.
```json
{
  "metadata": {
    "simulator": "MBPlib simulate",
    "simulator_version": "v0.6.0",
    "trace": "traces/SHORT_SERVER-1.sbbt.zst",
    "warmup_instr": 0,
    "simulation_instr": 1283891318,
    "exhausted_trace": true,
    "num_conditonal_branches": 162876464,
    "num_branch_instructions": 16056,
    "predictor": {
      "name": "MBPlib Gshare",
      "history_length": 25,
      "log_table_size": 18,
      "track_only_conditional": false
    }
  },
  "metrics": {
    "mpki": 3.312180665435421,
    "mispredictions": 4252480,
    "accuracy": 0.973891378192002,
    "simulation_time": 14.347951476,
    "num_most_failed_branches": 20
  },
  "predictor_statistics": {},
  "most_failed": [
    {
      "ip": 1995000000,
      "occurrences": 3231824,
      "mpki": 0.22370351444342426,
    },
    {
      "ip": 2148302608,
      "occurrences": 1638183,
      "mpki": 0.19969447289307085,
    },
    ...
  ],
  "errors": []
}

```

### Obtaining Traces

MBPlib uses a custom binary trace format called Simple Binary Branch Trace format (extension .sbbt). Although MBPlib can read traces compressed with multiple utilities, like `gzip` and `xz`, the best compression ratio and decompression speed is obtained with [`zstd`] (by a big margin).

You can download the training (223 traces) and evaluation (440 traces) workloads from the [Championship Branch Prediction 5] at https://webs.um.es/aros/tools/MBPLib_traces/cbp5_train/ and https://webs.um.es/aros/tools/MBPLib_traces/cbp5_eval/, respectively, and the 95 traces from the [3rd Data Prefetching Championship], which are based on the [SPEC CPU 2017] Benchmark, at https://webs.um.es/aros/tools/MBPLib_traces/dpc3/.

You can also create your own traces using the [SBBT tracer](/app/tracer), an instrumentation tool built on top of [PIN].

[`zstd`]: https://en.wikipedia.org/wiki/Zstd
[Championship Branch Prediction 5]: https://jilp.org/cbp2016/
[3rd Data Prefetching Championship]: https://dpc3.compas.cs.stonybrook.edu/
[SPEC CPU 2017]: https://www.spec.org/cpu2017/
[PIN]: https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html

## Documentation

The complete documentation is still a work in progress. Fortunately, it is very easy to learn by example. Take a look at the [utils] directory or the [examples library] from MBPlib if you want to learn which utilities MBPlib offers. For how to link against the library and create the executables, you can take a look at the [CMakeLists.txt](/example/CMakeLists.txt) file in the [example] directory.

[utils]: /include/mbp/utils
[examples library]: /include/mbp/examples
[example/traces]: /example/traces

### Predictor Interface

In MBPlib, a predictor must inherit from `mbp::Predictor`, which is the interface used for the simulator. This requires implementing at least the functions `predict`, `train` and `track`.

For instance, you can implement a gshare predictor using the class `mbp::i2` (from the utilities library) as a two-bit saturated counter and a `std::bitset` for the global history register (ghist).

```cpp
// File my_gshare.hpp
#include <array>
#include <bitset>

#include "mbp/core/predictor.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "mbp/utils/indexing.hpp"

template <int H = 15, int T = 17>
struct Gshare : mbp::Predictor {
  std::array<mbp::i2, (1 << T)> table;
  std::bitset<H> ghist;

  uint64_t hash(uint64_t ip) const {
    return mbp::XorFold(ip ^ ghist.to_ullong(), T);
  }

  bool predict(uint64_t ip) override { return table[hash(ip)] >= 0; }

  void train(const mbp::Branch& b) override {
    table[hash(b.ip())].sumOrSub(b.isTaken());
  }

  void track(const mbp::Branch& b) override {
    ghist <<= 1;
    ghist[0] = b.isTaken();
  }
};
```

### Calling the Simulator

With the previous code, you can generate an executable file that will run a simulation by calling `mbp::Simulate(predictor, simargs)` with an instance of your predictor and an instance of `mbp::SimArgs`, a structure containing the name of the trace, the amount of instructions to be used as warmup and the amount of simulation instructions after that. The simulation will return an `mbp::json` object, which you can then handle. The simulation arguments can be parsed from the commandline arguments with the function `mbp::ParseCmdLineArgs(argc, argv)`, which will produce an interface like that of the [example] folder executables.

But since having that simple command line interface and printing the json object to standard output is a very typical usecase, `mbp::SimMain` can do all those steps for you. Thus, the simplest of mains will look like the following extract of code.

```cpp
#include "mbp/sim/simulator.hpp"
#include "my_gshare.hpp"

static Gshare<25, 18> gshare;

int main(int argc, char** argv) {
  return mbp::SimMain(argc, argv, &gshare);
  // Equivalent to:
  // mbp::SimArgs clargs = mbp::ParseCmdLineArgs(argc, argv);
  // mbp::json output = mbp::Simulate(&gshare, clargs);
  // std::cout << std::setw(2) << output << std::endl;
  // return output["errors"].empty() ? 0 : mbp::ERR_SIMULATION_ERROR;
}
```

### Completing the Output

A predictor can optionally override the methods `metadata_stats()` and `execution_stats()`, which return a json object. The first allows the user to include information about the predictor being used in the output. This is useful when you plan to store the output of your experiments. The second is to include metrics specific to the predictor. For example, you could include how many times (per kilo byte instructions) you had to remplace an entry in the predictor, which can serve as a measure of the aliasing conflicts.

In the gshare class it would make sense to add the following code
```cpp
  mbp::json metadata_stats() const override {
    return {
        {"name", "My Own Gshare"},
        {"history_length", H},
        {"log_table_size", T},
    };
  }
```
