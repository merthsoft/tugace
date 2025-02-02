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

## Coming up
| Command | Description |
| - | - |
| GOSUB [number] | Goes to label [number], pushing the the next line onto the stack |
| RET | Pops a value off the stack and jumps to that location |
| STO [var] | Stores Ans to real var [var] |
| CIRCLE [radius] | Draws a circle of radius [radius] centered around the turtle |
| ELLIPSE {[radiusX],[radiusY] | Draws an ellipse of radius [radiusX],[radiusY] centered around the turtle |
| EVAL [code] | Evaluates arbitrary BASIC code in [code] |
| TURTLE [num] | Sets which turtle commands operate on |
| STACK [num] | Operates which stack commands operate on |
| TEXT [str] | Writes [str] to the screen at turtle position |
| STOP | Ends the program |
| SPEED [val] | Sets the turtle's auto-movement speed. Effective call FORWARD [val] each frame, evaluated when set |

## Additional shorthands
I added ':' as a shorthand for label. Proposed other shorthands are:

### Symbolic
| Symbol | Command |
| - | - |
| + | INC |
| - | DEC |
| { | PUSH |
| } | POP |
| small E | PEEK |
| -> | STO |
| . | GOTO |
| * | GOSUB |
| / | RET |
| ^ | FORWARD |
| ( | LEFT |
| ) | RIGHT |
| < | PUSHVEC |
| > | POPVEC |
| = | PEEKVEC |
| , | ANGLE |
| ^2 | SPEED |

### Non symbolic
| Symbol | Command |
| - | - |
| T | TURTLE |
| P | PEN |
| W | WRAP |
| C | COLOR |
| M | MOVE |
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
