<h1 align="center">MBPlib</h1>

MBPlib is a modular library for the development and simulation of branch predictors.
MBPlib is ready to use, but the documentation is still in progress.

## Using MBPlib

MBPlib is built using [CMake].
You can depend on MBPlib by the traditional `include_subdirectory` command.
You can take a look at the [example] folder.

[CMake]: https://cmake.org/
[example]: /example

## Example: Gshare predictor

In this section we will show what you can expect from MBPlib.

### Development

In MBPlib, a predictor must inherit from `mbp::Predictor`, which is the interface used for the simulator.
This requires implementing the functions `predict`, `train` and `track` at the very least.
MBPlib also offers utilities that are commonly used in branch prediction schemes, like two-bit saturating counters.

For instance, you can implement a gshare predictor using the class `mbp::i2` as a two-bit saturated counter
and `std::bitset` for the global history register (ghist).

```cpp
#include <array>
#include <bitset>

#include "mbp/sim/predictor.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "mbp/utils/indexing.hpp"
#include "nlohmann/json.hpp"

template <int H = 15, int T = 17>
struct Gshare : mbp::Predictor {
  std::array<mbp::i2, (1 << T)> table;
  std::bitset<H> ghist;

  uint64_t hash(uint64_t ip) const {
    // Shift left the history to make the least significant address bits
    // be xored with the least amount of history bits.
    static_assert(H + (T - (H % T)) <= sizeof(unsigned long long) * 8);
    return XorFold(ip ^ (ghist.to_ullong() << (T - (H % T))), T);
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

With the previous code, you can generate an executable file that will run a simulation via the following code.

```cpp
#include "mbp/examples/mbp_examples.hpp"
#include "mbp/sim/simulator.hpp"

Gshare<25, 18> gshare_64KB;

mbp::Predictor* const mbp::branchPredictor = &gshare_64KB;

int main(int argc, char** argv) { return mbp::Simulation(argc, argv); }
```

### Input and Output

MBPlib uses a custom binary trace format that allows fast simulation.
The traces can also be compressed.
In that case, MBPlib works best if the traces are compressed using zstd.
MBPlib outputs in json format,
which is easily readable by both humans and machines.

### Completing the Output

A predictor can optinally override the methods `metadata_stats()` and `execution_stats()`, which return a json object.
The first allows the user to include information about the predictor being used in the output.
This is useful when you plan to store the output of your experiments.
The second is to include metrics specific to the predictor.
For example, you could include how many times (per kilo byte instructions) you had to remplace an entry in a Tage predictor,
which can serve as a measure of the aliasing conflicts.

For the example at hand we can add the following code inside our `Gshare` class.

```cpp
  mbp::json metadata_stats() const override {
    return {
        {"name", "My Own Gshare"},
        {"history_length", H},
        {"log_table_size", T},
    };
  }
```

Running the standard simulator with the command `./build/gshare_64KB traces/SHORT_SERVER-1.sbbt.zst` will produce the output

```json
{
  "metadata": {
    "simulator": "MBPlib std sim for cnd branches v0.4.0",
    "trace": "traces/SHORT_SERVER-1.sbbt.zst",
    "warmup_instr": 0,
    "simulation_instr": 1283944652,
    "exhausted_trace": true,
    "num_conditonal_branches": 162876464,
    "num_branch_instructions": 16056,
    "predictor": {
      "name": "MBPlib Gshare",
      "history_length": 25,
      "log_table_size": 18,
    }
  },
  "metrics": {
    "mpki": 3.312043080187229,
    "mispredictions": 4252480,
    "accuracy": 0.973891378192002,
    "num_most_failed_branches": 36,
    "simulation_time": 13.186420588
  },
  "predictor_statistics": {},
  "most_failed": [
    {
      "ip": 1995000000,
      "mpki": 0.22369422198426667,
      "accuracy": 0.9111303709607949
    },
    {
      "ip": 2148302608,
      "mpki": 0.19968617774966207,
      "accuracy": 0.8434936756149954
    },
    ...
  ]
}

```

## How to Start

Since the documentation is lacking, you will have to learn by example.
Take a look at the [utils] direrctory or the multiple [examples] from MBPlib if you want to learn which utilities MBPlib offers.
For how to link against the library and create the executables, you can take a look at the [example] directory.
Note that knowing a little bit of CMake will be helpful.

In order to run any experiment, you will need traces in SBBT format.
You can find an example trace from the Championship Branch Prediction 5 suite under [example/traces].
If you want to request the full set of traces from the Championoship Branch Predictor 5 translated for MBPlib,
you can write me at [emilio.dominigueuzs@um.es](mailto:emilio.dominguezs@um.es).
We will upload them in the near future.

[utils]: /include/mbp/utils
[examples]: /include/mbp/examples
[example/traces]: /example/traces
