# Conventions
## Naming Conventions
For naming convention we will be using GTK+, summarized as follows:
  1. All **macros and constants** in caps: ```MAX_BUFFER_SIZE, TRACKING_ID_PREFIX```.
  2. **Struct** names and typedef's in camelcase: ```GtkWidget, TrackingOrder```.
  3. **Functions** that operate **on structs**: classic C style: ```gtk_widget_show(), tracking_order_process()```.
  4. **Pointers**: nothing fancy here: ```GtkWidget *foo, TrackingOrder *bar```.
  5. **Global variables**: we personally will be using underscore followed by name like ``` _num_child_process_alive```
  6. **Functions** that are there, but **shouldn't be called directly**:one or more underscores at the beginning: ```_refrobnicate_data_tables()```
  
## C Conventions
For C convention we will be using [Linux Kernel Conventions](https://www.kernel.org/doc/html/v4.10/process/coding-style.html), that can be summarized as follow (with some minor changes)
### Indentation
  1. 8 characters and real tab (no space)
  2. only exception is switch and case which should be aligned (no tabulation)
  3. don't use multiple statements/assignments on same line
### Breaking long line and strings
  1. break line and function header longer than 80 columns (unless it doesn't hide informations and breaking it does reduce readability)
  2. descentant should be substantially smaller and to the right
  3. don't break user message (cannot grep for them)
### Braces and spacing
  1. braces for all non-function statement blocks:
  ```c
  if (x == true){
    /*block*/
  }
  ```
  2. function have opening brace at beginning of the next line
  ```c
  int main(void)
  {
    /*block*/
  }
  ```
  3. line with closing brace contains only closing brace except for do while and else/else if
  ```c
  do {
    /*block*/
  }while(i<100);
  
  if (i == 0){
    /*block*/
  } else if (i < 0){
    /*block*/
  }else {
    /*block*/
  }
  ```
  4. do not use unnecessarily braces when is usable single statement (exept when only one branch of a conditional statement is a single statement)
  ```c
  if (i == 0)
    do_something();
  else
    do_something_else();
  
  if (i == 0){
    do_something();
  } else {
    /*block*/
  }
  ```
### Space
  *  avoid trailing whitespace
  1. use space after almost all keywords, like ```if, switch, case, for, do, while ```
  2. exept for: ``` sizeof, tipeof, alignof, __attribute``` that are similar to functions
  3. do not add space around parenthesized expression. Bad example: ```sizeof( file );```
  4. pointer symbol (* ) should be near the data/function name (ex. ```char *str```)
  5. use one space around binary/ternary expression like (``` =  +  -  <  >  *  /  %  |  &  ^  <=  >=  ==  !=  ?  : ```)
  6. but not for unary or pre/postfix increment or decrement and struct member operator(``` &  *  +  -  ~  ! x++ x-- ++x --x . ->```)
### Naming
  1. no mixed cased variable name (like ``` randomVariable```)
  2. **local** variable should be short and to the point ```tmp, i```
  3. **global** variable and function should have descriptive name (ex. ```_count_active_users()```)
  4. functions names should not contain the type that they return
### Functions
  1. shold be short and do **one thing** (should fit in 1 or 2 screens of 24 line and 80 columns)
  2. should have max 5-10 variables
  3. can have longer lines if the function is shorter
### Commenting
  1. don't try to explain how it works but **what** it does
  2. avoid comments inside a function (if it is too complex refer to the section 'Functions')
  3. put comments **before functions** explaining what it does and **why**
  4. leave space after every data declaration -> don't use ``` type var1, var2; ```
  5. use this format for multi-line comments:
  ```c
  /*
   * Comment starts here
   * Comment ends here
   */
  ```
### Allocating memory
  1. Memory allocation should be done like ``` p=malloc(sizeof(*p)) ```
  2. Dynamic array should be allocated like ``` p=calloc(n, sizeof(*p)) ```
### Typedef
  1. Lots of people think that typedefs help readability. Not so. They are useful only for:
  2. totally opaque objects (where the typedef is actively used to hide what the object is).
  3. Clear integer types, where the abstraction helps avoid confusion whether it is int or long.
  4. when you use to literally create a new type for type-checking
### Macros, Enums and RTL
  1. macro that make multiple statetements should use ```do{/*statements*/}while(0)```
  2. don't use a local variable in macro (with some defined name).  ex. ``` #define FOO(val) bar(index, val) ```
  3. don't affect flow. ex. ```#define FOO(x) return x```
  4. use parentheses around parameter and result. ex. ``` #define FOO(x) ((x) * 2) ```
### Printing messages
  1. messages should be concise and easy to understand
  2. don't have to be terminated with a period
  3. parentheses around numers should be avoided. Ex. ``` printf("The result is (%d)", res);```
  4. use DEBUG flag at compile time for debug message
### Function Return values and names
  1. If the name of a function is an action or an imperative command,the function should return an error-code integer.  If the name is a predicate, the function should return a "succeeded" boolean.
```C
add_work() //returns 0 for success or -EBUSY for failure
pci_dev_present() //returns 1 if it succeeds in finding a matching device or 0 if it doesn’t.
```
  2. Functions whose return value is the actual result of a computation, indicate failure by returning some out-of-range result. Typical examples would be functions that return pointers; they use NULL to report failure.
### Centralized exit of function
  1. can be used for common clean up (using goto to the last part of a function)
