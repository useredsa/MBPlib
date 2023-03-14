<h1 align="center">
  SBBT Tracer
</h1>

This folder contains the source code of the utility `sbbt_tracer`, a tool capable of creating SBBT traces of the control flow of a program execution.

## Compiling

This software should compile and run on any Unix platform. You need to download [Intel PIN tools] and have [`zstd`] installed. The variable `$PIN_ROOT` should point to the location of the PIN tools directory.

To compile the program type
```sh
make
```
The shared library will be created in `/obj-intel64/sbbt_tracer.so`.

[`zstd`]: https://en.wikipedia.org/wiki/Zstd
[Intel PIN tools]: https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html

## Running

To run the program (as with any other PIN tool) you have to execute
```sh
"$PIN_ROOT/pin" [pin_options ...] -t obj-intel64/sbbt_tracer.so [sbbt_tracer options...] -- <command> [command options...]
```
To have a detailed list of pin options or this utility options, you can use the option `-help` as a command option.

The most important options available to this PIN tool are listed in the following table.
| Option                | Usage |
|-----------------------|-------|
| -o <path>             | Output trace file basename (default: `trace`) |
| -details-file <path>  | Output details file with mnemonics basename. If not set, the file is not created (default: empty) |
| -skip <num>           | Number of instructions to skip before starting tracing (default: 0) |
| -length <num>         | Number of instructions to trace (default: until the program finishes) |

## Example

Executing
```sh
"$PIN_ROOT/pin" -t obj-intel64/sbbt_tracer.so -- ls /
```
will produce the file `trace.sbbt.zst` with the details of the control flow of the execution of the program `ls`. You can use the utility [`sbbt_cat`] to visualize the information gathered.
```sh
sbbt_cat trace.sbbt.zst | head
┌──────────┬──────────────────┬──────────────────┬────────────┬─────────┐
│ Inst Num │  Branch Address  │  Target Address  │   Opcode   │ Outcome │
└──────────┴──────────────────┴──────────────────┴────────────┴─────────┘
          2 0x00007f7a3179fed3 0x00007f7a317a0b20 DIR UCD CALL TAKEN
         29 0x00007f7a317a0b95 0x00007f7a317a0c28 DIR CND JUMP NOT_TAKEN
         35 0x00007f7a317a0bba 0x00007f7a317a0bda DIR UCD JUMP TAKEN
         37 0x00007f7a317a0bde 0x00007f7a317a0bc9 DIR CND JUMP TAKEN
         42 0x00007f7a317a0bd8 0x00007f7a317a0c28 DIR CND JUMP NOT_TAKEN
         44 0x00007f7a317a0bde 0x00007f7a317a0bc9 DIR CND JUMP TAKEN
         49 0x00007f7a317a0bd8 0x00007f7a317a0c28 DIR CND JUMP NOT_TAKEN
```

[`sbbt_cat`]: /../sbbt_cat/main.cpp
