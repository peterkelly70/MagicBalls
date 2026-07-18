# MC1 to MC2 converter

Initial command-line scaffold for translating a legally installed GOG copy of **Magic Carpet Plus** into an MC2-compatible data tree for MagicBalls.

The current executable only loads configuration, applies command-line overrides, validates paths and prints the selected conversion scope. Binary level conversion is not implemented yet.

## Configure

From the repository root:

```bash
cp .env.example .env
```

Edit `.env` and set:

```dotenv
MC1_PATH="/path/to/Magic Carpet Plus"
MC2_PATH="/path/to/Magic Carpet 2"
OUTPUT_PATH="/path/to/MagicBalls-MC1"
DEFAULT_LEVEL=0
VERBOSE=true
```

Precedence is:

1. command-line option
2. `.env` value
3. validation error

Neither source installation is modified.

## Build

```bash
cmake -S tools/mc1_to_mc2 -B tools/mc1_to_mc2/build
cmake --build tools/mc1_to_mc2/build
```

## Run

Use `.env` defaults:

```bash
tools/mc1_to_mc2/build/mc1_to_mc2
```

Convert a selected level with explicit overrides:

```bash
tools/mc1_to_mc2/build/mc1_to_mc2 \
    --mc1 "/path/to/Magic Carpet Plus" \
    --mc2-template "/path/to/Magic Carpet 2" \
    --output "/path/to/MagicBalls-MC1" \
    --level 0 \
    --verbose
```

Use a different environment file:

```bash
tools/mc1_to_mc2/build/mc1_to_mc2 \
    --env ./machine.env \
    --all
```

## Next implementation step

Add an MC1 reader that:

1. reads `LEVELS/LEVELS.TAB`
2. extracts the selected record from `LEVELS/LEVELS.DAT`
3. decompresses RNC method 1
4. validates the 38,812-byte MC1 level structure
5. reports terrain parameters and active entity counts

That probe should be completed before writing MC2 output files.
