<div id="table-of-contents">
<h2>Table of Contents</h2>
<div id="text-table-of-contents">
<ul>
<li><a href="#orgheadline6">1. Lunar Lions Programming Style Guide For C and C-like Languages</a>
<ul>
<li><a href="#orgheadline1">1.1. Naming Conventions</a></li>
<li><a href="#orgheadline2">1.2. Bracketing Conventions</a></li>
<li><a href="#orgheadline3">1.3. White Space</a></li>
<li><a href="#orgheadline4">1.4. Comments</a></li>
<li><a href="#orgheadline5">1.5. Structure of a file</a></li>
</ul>
</li>
</ul>
</div>
</div>

# Lunar Lions Programming Style Guide For C and C-like Languages<a id="orgheadline6"></a>

## Naming Conventions<a id="orgheadline1"></a>

-   Constants and #defines should be MACRO<sub>CASE, with underlines 
    separating words. As an aside, #define is preferable in C for constants
    since the const keyword really means readonly, but static constants are
    preferred in everything else. Refer to 
    <http://stackoverflow.com/questions/1674032/static-const-vs-define-vs-enum>
    for more information.
    -   `static const int FOO_BAR
        #define TIME 50`
-   Classes, structures and type definitions should use TitleCase
    -   `struct JimmyJohnsSandwich {
          bool delivered_quickly;
          double cost;
        }`
-   Local variables should be lowercase, including instantiations of a 
    class/struct. The exception to this rule is that named components (i.e. 
    FO-U, AV5-M, et cetera) will retain their case as you would write them.
    -   `JimmyJohnsSandwich number_10 = {
          .delivered_quickly = true,
          .cost = 3.50
        };`
-   Pointers and references should be placed next to the type, not the variable
    name.
    -   `int\* foo = 0xDEADBEEF;`
-   Enums should be named using TitleCase, and their members should be named 
    with snaking ALL_CAPS.
    -   `enum CardinalDirections {
          NORTH,
          SOUTH,
          EAST,
          WEST
        }`
-   Functions names should be all lowercase.
    -   `int fiddle_doodad(int& diddly_bopper)
        {
          *\* code goes here \**
        }`

## Bracketing Conventions<a id="orgheadline2"></a>

-   For function declarations, always place the opening bracket on a newline.
    -   `void empty_platitude(void)
        {
          printf("Keep up the good work!\n");
        }`
-   For literally everything else, the opening bracket goes on the same line.
-   For conditional statements, always, always, ALWAYS use brackets, every
    single time.
    -   `if (qux) {   *\* Good \**
            baz += 2;
        }
        if (SOME_CONSTANT == quux)
            baz = baz/2 + 7; *\* NO! BAD! \**`

## White Space<a id="orgheadline3"></a>

-   For readability purposes, please occasionally break up blocks of code
     using blank lines. Good places to do so are between variable declarations
    and control structures like conditional statements.
-   Put a space between operands and operators. 
    -   `int j = i % 5 + 2; *\* Good \**`
    -   `int k=j\*9/(i-10); *\* Bad \**`
-   Don't put a space immediatley after an opening parenthesis or between the
    last term and the closing parenthesis.
    -   int quuux(double example)
-   Commas follow the preceding term with no whitespace between, and are 
    followed either by a space or a new line.
-   Always use 4 spaces of indentation.
    -   `int main(void)
        {
            for(int i = 0; i < 10; i++) {
                printf!("i has appeared %d times.\n", i + 1);
            }
            return 0;
         }`
-   Line length should be limited to 120 characters. It's acceptable to 
    write a few more characters than 120. 
    -   Do not reduce the length of names of variables to make a piece of code 
        fit on a single line.
    -   Disregard the limit when it comes to user visible strings, since breaking
        those strings makes it harder to search for them later.

## Comments<a id="orgheadline4"></a>

-   Always use block style (*\* \**) comments, do not use single line (//) comments
    -   `/\* Superior
        
        -   commenting
        -   style
        
        \*/`
        
        `// Don't do this`
-   Short comments on the same line as code are acceptable as long as they
    don't take up too much space. Make sure to put an adequate amount of space
    between them and the code, and if more than 1 appears to make them start on
    the same column.
    -   `*\* Brazenly stolen example from NASAs C Style Guidelines \** 
              double ieee_r[];          *\* array of IEEE real\*8 values \**
              unsigned char ibm_r[];    *\* string of IBM real\*8 values \**
              int count;                *\* number of real\*8 values     \**`
-   Each file should state its name, briefly state its purpose, and list files 
    which undergo I/O operations in the file prolog.
-   Functions should also have prologs which state the general purpose of the
    function, and possible errors. The parameters of the function should be
    named so that their identity and purpose are abundantly clear.

## Structure of a file<a id="orgheadline5"></a>

1.  Comment prolog
2.  Include directives
3.  Define directives
4.  Type definitions and Enums
5.  External definitons
6.  Main function
7.  other functions
