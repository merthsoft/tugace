# Tuga

2025, Merthsoft Creations

Tuga is a turtle programming language designed around the TI-84+CE graphing calculator. It's meant to be simple to learn and read while still providing enough power to make cool fractals and learn about programming. The specification itself is "missing" many things that would otherwise be necessary--this is intentional, this language is designed around existing functionailty on the TI-84+CE, and will reuse what's possible.

TUGA is the accompanying interpreter. It uses the built-in program format as its container, and reuses the operating system variables and math evaluation. It aims to provide a turtle language that can be programmed on the calculator easily, and integrates with the OS as seamlessly as possible.

## Links

Github: https://github.com/merthsoft/tugace/

Alpha: https://merthsoft.com/tuga/Tuga.zip

## IN CONSTANT FLUX

The only constant is change. But right now, especially, things can change very quickly. This document may be out of date with the code base, but I'll do my best.

## Pre-requisites

You'll need a TI-84+CE that's had artifice run on it, as well as ASMHOOK.

## Using TUGA

Send TUGA and the sample programs to your calculator. Run TUGA. You should see a shell showing you the available programs. Use the arrow keys to select a program and press enter to run it. Alternatively, most programs have self-launching, so you can run the program from the homescreen.

### Program Execution

Once the program has finished, it will be in the "DONE" state. Alternatively, while TUGA is running, you can press "MODE", "DEL", or "CLEAR" to be put in the done state. This is the same as a program running the "STOP" command.

Once in the done state, press "MODE", "DEL", or "CLEAR" to quit, or "ENTER" to restart the program.

While the program is running, press "ENTER" to pause execution.

## Header

### Minimal
Programs should start with `0TUGA` to be recognized by the shell. It's recommended that you at the very least include `:Return` after `0TUGA`, so it doesn't function as a TI-BASIC program. Additionally, it supports self-launching, so you can still do:

### Self-launching
```TUGA
0TUGA:"PROGNAME":Asm(prgmPROGNAME:Return
```

Or any amount of BASIC code on the first line that you want, really.

#### Shell

##### Iconless

```TUGA
0TUGA:"PROGNAME":Asm(prgmTUGA:Return
"Description
```

##### Icon

```TUGA
TUGA:"PROGNAME":Asm(prgmTUGA:Return
:HEX ICON
"Description
```

Icon is a 16x16 BASIC palette sprite (similar to DCS header).

### Sub-programs

For sub-programs, just leave out "0TUGA", and make the first line just "Return" (so it doesn't work as a BASIC program).

## Commands

Commands are typed in all caps and parameters are evaluated using the OS evaluation. Commands with `{` are expecting a list. Commands that don't accept lists can take a list, and only the first element will be used.

Because TUGA uses the OS evaluation, your parameters can be formulas--and they can even be complex (imaginary value is truncated before passed on to the turtle itself). This means you can do `COLOR randInt(0, 255` to set the turtle to a random color. Additionally, the variables are the system variables. If you do `1->A` before running your program, and your program is `INC A`, then on the homescreen of TI-OS you check the value of `A`, it will be `2`.

Comments start with `"` and are skipped entirely.

### Implemented

### Turtle control

| Command | Description |
| - | - |
| TURTLE [num] | Sets which turtle commands operate on |
| COLOR [color] | Sets the turtle color to [color] |
| PEN [status] | Sets the turtle pen status to [status] |
| WRAP [status] | Sets the turtle wrap status to [status] |
| FORWARD [amt] | Moves the turtle forward by [amt] |
| LEFT [amt] | Turn left by [amt] degrees |
| RIGHT [amt] | Turn right by [amt] degrees |
| MOVE {[X],[Y] | Moves to [X, Y] |
| ANGLE [amt] | Sets angle to [amt] degrees |
| INIT | Initializes the turtle, moving it the center of the screen, setting the color to 255, and setting the pen to 1 |

### Graphics

| Command | Description |
| - | - |
| CLEAR [color] | Clears the screen with [color] |
| FILL | Does a flood fill at the turtle's location of the current color |
| FADEOUT [step] | Fades out at a rate of [step] |
| FADEIN [step] | Fades in at the rate of [step] |
| INIT | Resets the current turtle to the initial conditions |
| RECT {[w],[h] | Draws a rectangle with upper-left corner at the turtle's location that's [w] by [h] |
| CIRCLE [radius] | Draws a circle of radius [radius] centered around the turtle |
| ELLIPSE {[radiusX],[radiusY] | Draws an ellipse of radius [radiusX],[radiusY] centered around the turtle |
| ELLIPSE {[x],[y],[radiusX],[radiusY] | Draws an ellipse of radius [radiusX],[radiusY] centered around (x, 1) |
| TEXT [str] | Writes [str] to the screen at turtle position |
| DRAWSCREEN | Draws to the screen |
| DRAWBUFFER | Draws to the buffer |
| SWAPDRAW   | Swaps the screen and buffer |

#### Sprites and Tilemaps

| Command | Description |
| - | - |
| DEFSPRITE "[index],[w],HEX SPRITE DATA" | Defines a sprite, allocating it as needed, adding at [index] in the sprite dictionary. Height is assumed by length. |
| SPRITE [num] | Sets the turtle to be sprite [num] in the sprite dictionary |

#### Palettes

| Command | Description |
| - | - |
| PALETTE {[num],[param1],[param2] | Sets the palette. Currently supports 0 for default |
| PALSHIFT | Shifts the palette by 1 |

##### Built-in palettes

| Number | Palette |
| - | - |
| 0 | Default |
| 1 | Rainbow |
| 2 | Monochrome |

### Control flow

Label names should be all-caps.

| Command | Description |
| - | - |
| LABEL [name] | Declares label [name] |
| GOTO [name] | Goes to label named [name] |
| LABEL [number] | Declares label [number] |
| GOTO [number] | Goes to label number [number] |
| IF [val] | Skips the next line if [val] is 0 |
| GOSUB [name] | Goes to label named [name], pushing the the next line onto the system stack |
| GOSUB [number] | Goes to label [number], pushing the the next line onto the system stack |
| RET | Returns to where you jumped from, popping that value off the system stack |
| STOP | Ends the program |

#### Input

| Command | Description |
| - | - |
| GETKEY | Gets the current key to Ans |
| GETKEY [var] | Gets the current key to [var] |
| KEYDOWN [key] | Sets ans to 1 if the key is down, otherwise 0 (BASIC key code) |
| KEYUP [key] | Sets ans to 1 if the key is UP, otherwise 0 (BASIC key code) |
| IFKEYDOWN [key] | Processes the next line if the key is down (BASIC key code) |
| IFKEYUP [key] | Processes the next line if the key is UP (BASIC key code) |

### Stack

| Command | Description |
| - | - |
| STACK [num] | Operates which stack commands operate on |
| PUSH [val] | Pushes [val] onto the stack |
| PUSH [list] | Pushes a list onto the stack |
| POP | Pops off the stack into Ans |
| POP [var] | Pops off the stack into [var] |
| PEEK | Peeks at the stack into Ans without changing the stack pointer |
| PEEK [var] | Peeks at the stack into [var] without changing the stack pointer |
| PUSHVEC | Pushes the entire turtle vector (x, y, angle) onto the stack |
| POPVEC | Pops the turtle vector off the stack, setting the current turtle's values to that |
| PEEKVEC | Peeks the turtle vector from the stack, setting the current turtle's values to that without changing the stack pointer |

### Variables

| Command | Description |
| - | - |
| ZERO [var] | Sets real var [var] to zero |
| INC [var] | Increments real var [var] by one |
| INC {[var],[val] | Increments real var [var] by [val] |
| DEC [var] | Decrements real var [var] by one |
| DEV {[var],[val] | Decrements real var [var] by [val] |
| STO {[var],[val] | Stores [val] to real var [var] |

### Proposed

#### Misc

| Command | Description |
| - | - |
| STO [var] | Stores Ans to [var] |
| EVAL [code] | Evaluates arbitrary BASIC code in [code] |
| SPEED [val] | Sets the turtle's auto-movement speed. Effective call FORWARD [val] each frame, evaluated when set |
| ASM [code] | Runs ASM [code] |
| FORWARD {[X],[Y] | Disregards angle, and just moves to {x + [X], y + [Y]} |

#### Control flow (propsed)

| Command | Description |
| - | - |
| RESET | Reset the program |
| JNZ {[val],[label] | Jumps to [label] if [val] isn't zero |
| DJNZ {[var],[label] | Decrements [var] and jumps to [label] if the result is non-zero |
| PROG [program] | Runs [program] as a Tuga program |

#### Stack (propsed)

| Command | Description |
| - | - |
| STACK -1 | Select the system stack for stack commands |
| PUSHPC | Pushes PC of the command after this one onto the system stack |
| POP [list] | If [list] is a list of real vars, pops into those real vars, if it's a list var, pops into that list var |
| PEEK [list] | If [list] is a list of real vars, pops into those real vars, if it's a list var, pops into that list var |
| POP {[count] | Pops [count] items off the stack returned as a list in ans |
| PEEK {[count] | Peeks at [count] items from the stack returned as a list in ans |
| PEER [index] | Returns the stack value in [index] in ans |
| PEER {[index],[count] | Returns the value in [index] through [index]+[count] |
| PROD {[index],[val0],[val1]... | Writes [valx] to stack at [index + x] |

#### Drawing commands

| Command | Description |
| - | - |
| LINE {[x1],[y1],[x2],[y2] | Draws a line from (x1, y1) to (x2, y2) |
| RECT {[x1],[y1],[w],[h] | Draws a rectangle at (x1, y1) that's w by h |
| FPS [val] | Turns off the FPS counter if [val] is 0 |

#### Screen commands

| Command | Description |
| - | - |
| BLITSCREEN | Blits the screen to the buffer |
| BLITBUFFER | Blits the buffer to the screen |

#### Sprites and Tilemaps (proposed)

| Command | Description |
| - | - |
| LOADSPRITES "[APPVAR] | Loads a sprite dictionary from [APPVAR] |
| SPRITE {[num],[x],[y] | Draws sprite [num] at [x],[y] |
| DEFTILEMAP "[w],HEX TILEMAP DATA" | Defines the hex tilemap data |
| TILEMAP {[x],[y],[map_x],[map_y],[view_width],[view_height] | Draws a tilemap at [x],[y] on the screen, offset at [map_x],[map_y] of [view_width]x[view_height] tiles |

#### Palettes (proposed)

| Command | Description |
| - | - |
| DEFPAL [list] | Sets the palette to [list] |
| PALSHIFT {[start], [amount], [length] | Shifts the palette starting at [start] ending at [start] + [length] by [amount], [amount] and [length] are optional and default to 1 and the full palette length |

##### Additional palettes

| Number | Palette |
| - | - |
| 3 | Monochromatic HSV saturation-spectrum, takes two additional params, color and direction |
| 4 | Monochromatic HSV value-spectrum, takes two additional params, color and direction |
| 5 | Random, takes direction |
| 6 | Spectrum, takes direction and hue skip |

### Additional shorthands

#### Symbolic

| Symbol | Command |
| - | - |
| + | INC |
| - | DEC |
| { | PUSH |
| } | POP |
| small E | PEEK |
| [u] | PEER |
| [v] | PROD |
| e | EVAL |
| -> | STO |
| : | LABEL |
| . | GOTO |
| * | GOSUB |
| / | RET |
| ^ | FORWARD |
| pi | MOVE |
| ( | LEFT |
| ) | RIGHT |
| [ | PUSHVEC |
| ] | POPVEC |
| = | PEEKVEC |
| ? | IF |
| imaginary i | COLOR |
| angle | ANGLE |
| single-quote | SPEED |
| ^r | SPRITE |
| X | TURTLE |
| 0 | ZERO |
| theta | STACK |
| < | FADEOUT |
| > | FADEIN |
| = | DEFSPRITE |
| != | DEFTILEMAP |

#### Unassigned (Main keys)

| Symbol | Command |
| - | - |
| ^2 | |
| ^-1 | |
| sin( | |
| cos( | |
| tan( | |
| log( | |
| ln( | |
| sin^-1( | |
| cos^-1( | |
| tan^-1( | |
| sqrt( | |
| 10^x | |
| [w] | |
| e^( | |

#### Unassigned (Test)

| Symbol | Command |
| - | - |
| <= | |
| >= | |

#### OS

Some additional conveniences from programming commands, space built in to token:

| Symbol | Command |
| - | - |
| Goto | GOTO |
| If | IF |
| Lbl | LABEL |
| Stop | STOP |
| getkey | GETKEY |
| LEFT | LEFT |
| PO[PV]EC | POPVEC |

## Sprite and Tilemap Data Formats

Hex strings are strings of characters, 0-9 and A-F. Every two characters is one byte.

### Sprite Data Format

Sprites are stored as a byte per pixel, from left to right, top to bottom (column major). In a sprite dictionary AppVar, for each sprite, there's a byte for width, a byte for height, and then the sprite data in column-major format.

### Tilemap Data Format




## Invocation of additional Tuga programs

Still needs a lot of design work. Probably when a program gets invoked, it gets loaded and copied after the executing program.

PC gets pushed and jumps to sub program, much like a GOSUB.

Sub program needs to call RET or else execution stops entirely.

Programs share stack.

We need a way to preserve labels between programs. This could be the system stack, but that's potentially a lot of wasted data if we do that naively (just pushing the label vector).

Labels could be shared between programs.
