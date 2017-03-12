# dolphin-memory-comparator
Simple C++ application to help compare memory addresses from [Dolphin Emulator](https://dolphin-emu.org/) memory dumps

## Useful for debugging
This program works with debugging in Dolphin to help compare two memory dumps where changes in RAM memory can occur (e.g. where and how memory allocates before and after a given operation).

Simply stop program/game execution at desired instruction, dump memory with dolphin and collect with dolphin-memory-comparator, move execution to another point in program/game, and run it again.

The program will output differences in the readable format of:<br>
<pre>
Address(Number in list)
-------
Hex value Before |ASCII representation|
Hex value After  |ASCII representation|
</pre>

## To Use
Since only standard libraries and headers are used, simply comple with `g++ dolphin-memory-comparator.cpp`

Run with `./a.out`

## Explicit filepath for ram.raw
If for some reason the program cannot find the Dolphin memory dump file `ram.raw` or you wish to specify your own filepath, simply include the filepath as a commandline argument like `./a.out /Users/SomeUser/path/to/ram.raw`
