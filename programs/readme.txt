Turtle Graphics Interpreter
Merthsoft Creations
Shaun McFall, 2015
shaunm.mcfall@gmail.com

Program TURTLE is an interpreter for a turtle graphics programming language.

To understand the concept, imaging you have a turtle. He has a pen tied to the end of his tail. You can tell him to move forward or backward,
and you can tell him to turn in a certain direction. If his tail is down, he'll draw a line on the ground in the direction you tell him to move.
That's the basic concept! You can draw all sorts of neat stuff with this. See the included programs for examples.

Below is the definition of the language and program format.

The turtle starts out with a black background and a pen color of white. He starts in the middle of the screen facing directly east.

== Included programs ==
SQUARE.8xp - Draws a simple white square. Demonstrates moving.
SPIRAL.8xp - Draws a color-changing spiral. Demonstrates loops, conditionals, and evaluation of BASIC code.
TREE.8xp - Draws a simple tree. Demonstrates how to use the stack.

== Program header ==
Each turtle program has a three-line header, which should have the following format:
TURTLE
"<a comment>
"<program name>→Str1:{1→ʟTUR:prgmTURTLE:Return

For example, if you have a tree program, you might have a header as such:
TURTLE
"Draws a simple tree, by Shaun McFall
"TREE→Str1:{1→ʟTUR:prgmTURTLE:Return

This header ensures a few things:
1) Quickly identifies this program as a turtle program
2) Gives you the chance to put some info in the comment
3) Lets the program be run directly without the prompting the user for which program to run

== Program format ==
Each line contains a command and its arguments. You cannot have multiple commands on a line. To define a comment,
start the line with a space. Note that comments will slow down the program.

== Basic commands ==
Parameters can be literal values (such as 1 or 90 or {20,20}), or they can be variables (such as A, or {G,H}), or they can be executable code (such as randInt(0,255)).

PEN <0|1> - Puts the pen up (0) or down (1)
COLOR <COLOR> - Sets the pen color. Uses the xLIBC palette
FORWARD <AMOUNT> - Moved the turtle forward
LEFT <ANGLE> - Turns left
RIGHT <ANGLE> - Turns right
MOVE {<X>,<Y>} - Sets the X,Y value of the turtle. Will draw a line if the pen is down
ANGLE <VAL> - Sets the angle of the turtle
CLEAR <COLOR> - Clears the screen to a certain color. Uses the xLIBC palette
CIRCLE {<Radius>,<Fill>} - Draws a circle of passed in radius. Set fill to 1 to draw it filled in, or 0 to leave it empty.

== Labels ==
Labels must be defined before they are used. By default there are only 50 labels. If you need more than this, you will need to manually expand the label array.
LABLE <LABEL> - Defines a label
GOTO <LABEL> - Jumps to a label

== Advanced commands ==
EVAL <EXECUTABLE LINE OF CODE> - Evaluates a line of code

Pop/peak into Z. There are 3 stacks by default, with 99 depth. If you need more than this, you will need to manually expand the stack matrix.
PUSH {<STACK>,<VAL>} - Pushes a value onto the stack. This is a post increment, so it updates the stack pointer after setting the value.
POP <STACK> - Pops a value off the stack
PEEK <STACK> - Gets the value on the stack without popping it off
PUSHVEC <STACK> - Pushes the turtle location and heading ({A,B,E}) onto the stack, incrementing the stack pointer by three
POPVEC <STACK> - Pops three values off the stack, storing the first as the turtle heading (E), the second as the turtle Y (B), 
                 and the third as the turtle X (A), decrementing the stack pointer by 3.
PEEKVEC <STACK> - Gets three values off the stack, storing the first as the turtle heading (E), the second as the turtle Y (B), 
                  and the third as the turtle X (A), without decrementing the stack pointer.

IF <VAL> - If VAL evaluates to 1, the next line will be performed, Otherwise, it's skipped.

== Defined variables you can use in Eval, or for debugging ==
A,B - X,Y Location of turtle
C - Color of the pen
D - Status of the pen
E - Angle of the turtle
θ - Current program line - Modify this at your own risk!

ʟLBL - Contains the line number the labels are defined at. You can use this to pre-fill labels 

[D] - Matrix which contains the stacks
ʟSP - The stack pointers

ʟTUR - Some other advanced variables:
ʟTUR(1) - Number of lines in the program - Modify this at your own risk!
ʟTUR(2) - Location of the first space in the current command being processed. Changing this won't do anything.
ʟTUR(3) - Old X location of the turtle when moving.
ʟTUR(4) - Old Y location of the turtle when moving. 
ʟTUR(5) - The parsed value of the command. Only sometimes used. Changing this won't do anything.
ʟTUR(6) - Whether to draw a line from the old locations ({ʟTUR(3),ʟTUR(4)}) to the current location ({A,B}). If this is 1, a line will be drawn even if the pen is up.

ʟARGS - For list-based arguments, this'll be the argument list. For example, for MOVE {5,6}, this will be {5,6}. Changing this won't do anything.

Strings used by this program:
Str0 - The current program name to read/write. When evaluating code, this will be θθTURTMP. Otherwise it'll be the name of the program being executed.
Str1 - The name of the program being executed
Str2 - Arguments of command pre-parsing
Str3 - The command being executed
Str9 - The current line being parsed. When in an EVAL command, this is the same as Str2