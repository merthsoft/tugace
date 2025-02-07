# TUGA Interpreter
Merthsoft Creations

# Commands

## Implemented
| Command | Description |
| - | - |
| COLOR [color] | Sets the turtle color to [color] |
| PEN [status] | Sets the turtle pen status to [status] |
| WRAP [status] | Sets the turtle wrap status to [status] |
| FORWARD [amt] | Moves the turtle forward by [amt] |
| LEFT [amt] | Turn left by [amt] degrees |
| RIGHT [amt] | Turn right by [amt] degrees |
| MOVE {[X],[Y] | Moves to [X, Y] |
| ANGLE [amt] | Sets angle to [amt] degrees |
| CLEAR [color] | Clears the screen with [color] |
| LABEL [number] | Declares label [number] |
|     :[number] | Same as LABEL [number] |
| GOTO [number] | Goes to label number [number] |
| PUSH [val] | Pushes [val] onto the stack |
| POP | Pops off the stack into Ans |
| PEEK | Peeks at the stack into Ans without changing the stack pointer |
| PUSHVEC | Pushes the entire turtle vector (x, y, angle, color, pen, wrap) onto the stack |
| POPVEC | Pops the turtle vector off the stack, setting the current turtle's values to that |
| PEEKVEC | Peeks the turtle vector from the stack, setting the current turtle's values to that without changing the stack pointer |
| IF [val] | Skips the next line if [val] is 0 |
| ZERO [var] | Sets real var [var] to zero |
| INC [var] | Increments real var [var] by one |
| INC {[var],[val] | Increments real var [var] by [val] |
| DEC [var] | Decrements real var [var] by one |
| DEV {[var],[val] | Decrements real var [var] by [val] |
| STO {[var],[val] | Stores [val] to real var [var] |
| GOSUB [number] | Goes to label [number], pushing the the next line onto the stack |
| RET | Pops a value off the stack and jumps to that location |
| STO [var] | Stores Ans to real var [var] |
| TURTLE [num] | Sets which turtle commands operate on |
| STACK [num] | Operates which stack commands operate on |
| FILL | Does a flood fill at the turtle's location of the current color |
| FADEOUT [step] | Fades out at a rate of [step] |
| FADEIN [step] | Fades in at the rate of [step] |
| PALETTE [num] | Sets the palette. Currently supports 0 for default |

## Coming up
| Command | Description |
| - | - |
| CIRCLE [radius] | Draws a circle of radius [radius] centered around the turtle |
| ELLIPSE {[radiusX],[radiusY] | Draws an ellipse of radius [radiusX],[radiusY] centered around the turtle |
| EVAL [code] | Evaluates arbitrary BASIC code in [code] |
| TEXT [str] | Writes [str] to the screen at turtle position |
| STOP | Ends the program |
| SPEED [val] | Sets the turtle's auto-movement speed. Effective call FORWARD [val] each frame, evaluated when set |
| GETKEY | Gets the current key to Ans |
| GETKEY [var] | Gets the current key to [var] |
| POP [var] | Pops into [var] instead of Ans |
| POP [num] | Pops a number of items off into a list in Ans |
| PUSH [list] | Pushes a list onto the stack |
| BUFFERMODE [mode] | Sets to draw to screen (0) or buffer (1) |
| SWAPSCREEN | Swaps the screen and buffer |
| BLITSCREEN | Blits the screen to the buffer |
| SPRITE [num] | Sets the turtle to be sprite [num] in the sprite dictionary |
| DEFSPRITE {[num],[width],[height],[data...]} | Defines a sprite in the sprite dictionary


## Additional shorthands
### Symbolic
| Symbol | Command |
| - | - |
| + | INC |
| - | DEC |
| { | PUSH |
| } | POP |
| small E | PEEK |
| -> | STO |
| , | LABEL |
| . | GOTO |
| * | GOSUB |
| / | RET |
| ^ | FORWARD |
| ( | LEFT |
| ) | RIGHT |
| < | PUSHVEC |
| > | POPVEC |
| = | PEEKVEC |
| angle | ANGLE |
| ^2 | SPEED |
| T | TURTLE |
| P | PEN |
| W | WRAP |
| C | COLOR |
| M | MOVE |
| 0 | ZERO |
| S | STACK |
| X | TEXT |
| E | ELLIPSE |

### OS
Some additional conveniences from programming commands, space built in to token:
| Symbol | Command |
| - | - |
| Goto | GOTO |
| If | IF |
| Lbl | LABEL |
| Stop | STOP |
