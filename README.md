# Simulation of L<sup>★</sup>, NL<sup>★</sup>, UL<sup>★</sup>, AL<sup>★</sup>, and AL<sup>★★</sup>

## Basic usage

After building the binary with `make`, the program can be called without any command line arguments to learn a random RAT2 instance by L<sup>★</sup>, NL<sup>★</sup>, UL<sup>★</sup>, AL<sup>★</sup>, and AL<sup>★★</sup>.

```
$ ./automata
Seed: 856112064
Running L*... Finished after 0.189124seconds.
Automaton size: 17 states
Running NL*... Finished after 0.223801seconds.
Automaton size: 9 states
Running UL*... Finished after 0.285454seconds.
Automaton size: 12 states
Running AL*... Finished after 0.801777seconds.
Automaton size: 7 states
Running AL**... Finished after 1.21381seconds.
Automaton size: 7 states
```

The results of the learning algorithms are written to `seed_856112064.log`, where `856112064` is the seed used to generate the target automaton. A typical format of the logfile is:

```
Output of L* after 6 EQ and 1128 MQ:
digraph finite_state_machine {
[...]
}
Automata size: 17 states

Output of NL* after 15 EQ and 1666 MQ:
digraph finite_state_machine {
[...]
}
Automata size: 9 states

[...]
```

The data `digraph finite_state_machine { [...] }` describes the learning result and can be visualized using the command line tool `dot`:

```
$ dot -Tpng << EOF > graph.png
digraph finite_state_machine {
[...]
}
EOF
```

The above command writes a graphical representation of the automaton into the file `graph.png`.

## Command line parameters

The following command line parameters are supported.

|Parameter |Effect |
|--- |--- |
|`--algorithm <alg>`| The parameter `<alg>` should be one of the following:<ul><li>`L`     run Angluin's L<sup>★</sup> algorithm</li><li>`NL`    run NL<sup>★</sup> algorithm</li><li>`UL`    run UL<sup>★</sup> algorithm</li><li>`AL`    run AL<sup>★</sup> algorithm</li><li>`ALP`   run AL<sup>★★</sup> algorithm (includes a run of AL<sup>★</sup> since running AL<sup>★★</sup> until the first test for residuality is equivalent to a run of AL<sup>★</sup>)</li><li>`ALL`   run all algorithms mentioned above [default]</li><li>`noUL`  run all algorithms except UL<sup>★</sup></li>|
|`--rat1` | Learn a random automaton generated by RAT1. Such automata may be quite complex and oracle simulation may need huge amounts of RAM and computing time.|
|`--rat2` | Learn a random automaton generated by RAT2 [default]. |
|`--seed <seed>` | Use a specific seed (unsigned 32bit integer number) instead of a randomly chosen one for reproducibility. Note that depending on the implementation of `rand()` of the compiler, results may vary nonetheless.|
|`--regex <s> <exp>` | Disable randomness and define a specific target language. Output will be written to `regex.log` instead of `seed_<seed>.log`. The parameter `<s>` defines the alphabet sigma and the parameter `<exp>` defines the regular expression describing the target language. Only the following meta characters are supported: `( ) ? + * . \| [ ] \`. Inside of `[ ]`, these meta characters are supported: `- ^`.<br>Example: `./automata --regex abc 'b*\|[^b]+'`|
|`--ce <examples>` | Counter examples that shall be provided to the learner. Multiple counter examples are separated by comma. Although this parameter is also supported for targets generated at random, its usage makes sense only if a target language is defined explicitly by the `regex` parameter.|
|`--help`|Show help message|

## Examples

Learn the target automaton described by seed `856112064` (results will be written to `seed_856112064.log`):

```
./automata --seed 856112064
```

Learn the non-residual counter example of AL<sup>★</sup> (result will be written to `regex.log`):

```
./automata --algorithm AL --regex abcdefghijklmnopqr '(c(n|k|f|fp)|d(o|l|f|fp)|b(n|j|fp|fq|fg(g|r))|e(o|m|fp|fq|fg(g|r))|hq|i(g|r)|afp|afgg|an|ao|hg(g|r))g*' --ce afp,dl,an,ck,ao,em,bj,hq,ir
```

Run AL<sup>★★</sup> on the counter example (result will be written to `regex.log`):

```
./automata --algorithm ALP --regex abcdefghijklmnopqr '(c(n|k|f|fp)|d(o|l|f|fp)|b(n|j|fp|fq|fg(g|r))|e(o|m|fp|fq|fg(g|r))|hq|i(g|r)|afp|afgg|an|ao|hg(g|r))g*' --ce afp,dl,an,ck,ao,em,bj,hq,ir
```

Run the learning algorithms L<sup>★</sup>, NL<sup>★</sup>, and AL<sup>★★</sup> against an infinite number of RAT2 instances, but limit time and resource consumption of each run to 16GiB of memory and 12h of CPU time. The results will be written to a file named `seed_<seed>.log` each:

```
while true; do (ulimit -v 16777216 -t 43200; ./automata --algorithm noUL >> automata.log); done
```

## Scripts

In this section, the scripts from the folder `scripts` are described.

### automata.bash

The script `automata.bash` runs the last example from the previous section, learning automata with limited resources in an infinite loop. The log is written to `automata_$1.log`, where `$1` is the first command line argument supplied to the script.

### parallel.bash

Run `automata.bash` in parallel by starting a number of instances given as first command line argument.

### process\_logs\_non-smoothed.php

Parse `seed_<seed>.log` logfiles to extract the size of the DFAs, NFAs, and RAFAs subject to the minimal DFA. If a run has been aborted before AL<sup>★★</sup> has successfully learned the RAFA, the entire logfile is ignored, evenif the runs of L<sup>★</sup> or NL<sup>★</sup> have been finished successfully. The result is printed to `stdout`.

### process\_logs.php

This is an updated version of `process_logs_non-smoothed.php` which smooths the result by local means.

## Results

The `seed_<seed>.log` logfiles of our run of the `parallel.bash` script can be found in the `RAT2` folder. Additionally, results from a run with `--rat1` parameter (adapt `automata.bash` script accordingly) are located in the `RAT1` folder.

When producing our experimental results, we ran `process_logs.php` against these logfiles:
```
cd RAT2
php ../scripts/process_logs.php
```