# Differentitator

## Run
Program takes file with the following structure:
```
Equation itself - string
Tailor arguments   - pow[int] and point[double]
Graphics arguments - left border[int] and right one[double]
Tangent argument - point[double]
```
For example:
```
cos(5*x^3)^2*sin(3*x)
3 1.5
-2 2
12.3
```
This example will print tailor series in point 1.5 till o(x^3), graphic in x range = [-2:2], tangent equation in point 12.3 and in the end, you will see differential of given equation with all the transformations.

COMPILE:
> make

RUN:
> ./Diff [name of file with arguments]


## Info
This is my realization of basic math problem: differentiation, tailor rows, tangent equations and even graphics. ~~Unfortunately, now my differentiator parses equations only full bracket sequences. But I'm looking forward to rewrite it using recursive descend ([you can check an example here](https://github.com/ThreadJava800/Recursive-descend))~~ DONE.

Here are the rules for syntax analysis:
```
G    = E '\0' | '\n'
E    = T{['+' |  '-']T}*
T    = ST{['*' | '/']ST}*
ST   = P{[^]P}*
P    = '(' E ')' | X
X    = ['a'-'z' | sin P | cos P | ln P] | N
N    = ['0'-'9']+
```

My program also generates a .tex file with all the transformations done on equation. To reduce amount of writing in pdf file, I also realized a function that replaces similar and big subtrees with letters.

Here are the list of functions that you can call (others are just technic):
> DiffNode_t* openDiffFile(char* filenName, [optional]char* texName)

This function takes name of file with equation and parses it. As a result function returns a pointer to root of graph representation of equation. You can also provide latex file name for logging (the default is "zorich.txt").

> void tailor(DiffNode_t* node, int pow, double x0)

Prints tailor row for equ, represented as atree (that is got from the previous function). Except for node takes pow - decomposition order and x0 - point to build tailor row in.

> void equTangent(DiffNode_t* node, double x0)

Prints equation of tangent to function in point to latex file. Takes root of tree and point (x0).

> double funcValue(DiffNode_t* node, double x)

Returns value of fucntion in point. Takes pointer to root and point (x).

> int diffToTex(DiffNode_t* startNode)

Prints tree representation of equation to latex file. (The name of file was provided in openDiffFile()).

> int equDiff(DiffNode_t* start)

The main function that differentiates tree. [ATTENTION] it changes the tree you provided. If you want to kepp a copy of a start tree, use nodeCopy function

> DiffNode_t* nodeCopy(DiffNode_t* nodeToCopy)

Takes node and returns a pointer to its copy.

> void easierEqu(DiffNode_t* start)

This function makes tree smaller. It makes basic operations on constants and removes nodes that are multiplied with zero.

> void drawGraph(DiffNode_t* node)

Generates graphic of equation and puts image to latex file. (using gnuplot library). Takes only pointer to root of tree.

> void graphDump(DiffNode_t *node)

Opens a graphic dump of equation representation graph (using graphViz library).