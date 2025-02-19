# Tuga

2025, Merthsoft Creations

Tuga is a turtle programming language designed around the TI-84+CE graphing calculator. It's meant to be simple to learn and read while still providing enough power to make cool fractals and learn about programming. The specification itself is "missing" many things that would otherwise be necessary--this is intentional, this language is designed around existing functionailty on the TI-84+CE, and will reuse what's possible.

TUGA is the accompanying interpreter. It uses the built-in program format as its container, and reuses the operating system variables and math evaluation. It aims to provide a turtle language that can be programmed on the calculator easily, and integrates with the OS as seamlessly as possible.

## IN CONSTANT FLUX

The only constant is change. But right now, especially, things can change very quickly. This document may be out of date with the code base, but I'll do my best.

## Header

There are two headers the are designed:

```TUGA
TUGA:Return
```

This header is the most basic. This marks the program as a TUGA program, and tells TI-BASIC not to execute anything.

```TUGA
TUGA:"PROGRAM":prgmTUGA:Return
Description of the program
```

`PROGRAM` should be replaced with the name of your TUGA program. This tells TI-BASIC to launch TUGA with the program name in Ans, so it knows what to launch. The first line is a comment that can be optionally used by shells to display a description.

Additionally, the DCS header is supported, to give your programs a nice icon. A full TUGA+DCS header looks as follows:

```TUGA
:DCS
"33333333333333333333333BB33333333333B3B33B3B3333333B3B3333B3B33333B3333333333B33333B33333333B33333B3333333333B333B333333333333B33B333333333333B333B3333333333B33333B33333333B33333B3333333333B33333B3B3333B3B3333333B3B33B3B33333333333BB33333333333333333333333
TUGA:"SNOW":Asm(prgmTUGA:Return
"Draws a Koch snowflake
```


### Current implementation

As of right now, the code does not verify the header, and it takes for granted there will be a description comment. Also, the program name is hardcoded rather than coming from ans.

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

#### Palettes

| Command | Description |
| - | - |
| PALETTE {[num],[param1],[param2] | Sets the palette. Currently supports 0 for default |
| PALSHIFT | Shifts the palette by 1 |

##### Built-in palettes

| Number | Palette |
| - | - |
| 0 | Default, takes direction |
| 1 | Rainbow, takes direction |

### Control flow

| Command | Description |
| - | - |
| LABEL [number] | Declares label [number] |
| GOTO [number] | Goes to label number [number] |
| IF [val] | Skips the next line if [val] is 0 |
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

#### Control flow (propsed)

| Command | Description |
| - | - |
| RESET | Reset the program |
| JNZ {[val],[label] | Jumps to [label] is [val] isn't zero |
| DJNZ {[var],[label] | Decrements [var] and jumps to [label] if the result is non-zero |
| PROG [program] | Runs [program] as a TUGA program |

#### Stack (propsed)

| Command | Description |
| - | - |
| STACK -1 | Select the system stack for stack commands |
| PUSHPC | Pushes PC of the command after this one onto the system stack |

#### Drawing commands

| Command | Description |
| - | - |
| CIRCLE [radius] | Draws a circle of radius [radius] centered around the turtle |
| ELLIPSE {[radiusX],[radiusY] | Draws an ellipse of radius [radiusX],[radiusY] centered around the turtle |
| ELLIPSE {[x],[y],[radiusX],[radiusY] | Draws an ellipse of radius [radiusX],[radiusY] centered around (x, 1) |
| LINE {[x1],[y1],[x2],[y2] | Draws a line from (x1, y1) to (x2, y2) |
| RECT {[x1],[y1],[w],[h] | Draws a rectangle at (x1, y1) that's w by h |
| FPS [val] | Turns off the FPS counter if [val] is 0 |
| TEXT [str] | Writes [str] to the screen at turtle position |

There're no text or string mechanisms right now. `TEXT` will require that.

#### Screen commands

| Command | Description |
| - | - |
| DRAWSCREEN | Draws to the screen |
| DRAWBUFFER | Draws to the buffer |
| SWAPDRAW   | Swaps the screen and buffer |
| BLITSCREEN | Blits the screen to the buffer |
| BLITBUFFER | Blits the buffer to the screen |

#### Sprites

| Command | Description |
| - | - |
| SIZESPRITE {[num],[width],[height] | Sets the size of a sprite in the sprite dictionary. This should be called first as it initializes the sprite. |
| DEFSPRITE "HEX SPRITE DATA" | Defines the most-recently-sized sprite in the sprite dictionary with the hex sprite string |
| LOADSPRITES "APPVAR | Loads a sprite dictionary from APPVAR |
| SPRITE [num] | Sets the turtle to be sprite [num] in the sprite dictionary |
| SPRITE {[num],[x],[y] | Draws sprite [num] at [x],[y] |

#### Tilemaps

| Command | Description |
| - | - |
| SIZETILEMAP {[width],[height] | Sets the size of the tilemap to [width]x[height] |
| DEFTILEMAP "HEX TILEMAP DATA" | Defines the hex tilemap data |
| TILEMAP {[x],[y],[map_x],[map_y],[view_width],[view_height] | Draws a tilemap at [x],[y] on the screen, offset at [map_x],[map_y] of [view_width]x[view_height] tiles |

#### Palettes (proposed)

| Command | Description |
| - | - |
| PALETTE {[num],[param1],[param2] | Sets the palette. Currently supports 0 for default |
| PALSHIFT [amount] | Shifts the palette by [amount] |
| DEFPAL [list] | Sets the palette to [list] |
| PALSHIFT {[start], [amount], [length] | Shifts the palette starting at [start] ending at [start] + [length] by [amount], [amount] and [length] are optional and default to 1 and the full palette length |

##### Additional palettes

| Number | Palette |
| - | - |
| 2 | Monochromatic HSV saturation-spectrum, takes two additional params, color and direction |
| 3 | Monochromatic HSV value-spectrum, takes two additional params, color and direction |
| 4 | Grayscale, takes direction |
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
| -> | STO |
| : | LABEL |
| . | GOTO |
| * | GOSUB |
| / | RET |
| ^ | FORWARD |
| pi | MOVE |
| e | pen |
| ( | LEFT |
| ) | RIGHT |
| [ | PUSHVEC |
| ] | POPVEC |
| < | FADEOUT |
| > | FADEIN |
| = | PEEKVEC |
| ? | IF |
| imaginary i | INIT |
| angle | ANGLE |
| ^2 | SPEED |
| X | TURTLE |
| W | WRAP |
| C | COLOR |
| M | MOVE |
| 0 | ZERO |
| theta | STACK |

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

## Invocation of additional Tuga programs

Still needs a lot of design work. Probably when a program gets invoked, it gets loaded and copied after the executing program.

PC gets pushed and jumps to sub program, much like a GOSUB.

Sub program needs to call RET or else execution stops entirely.

Programs share stack.

We need a way to preserve labels between programs. This could be the system stack, but that's potentially a lot of wasted data if we do that naively (just pushing the label vector).

Labels could be shared between programs.
