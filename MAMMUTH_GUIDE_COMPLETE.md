# 🦣 MAMMUTH LANGUAGE - COMPLETEGUIDE

**Last Updated**: December 3, 2025

---


## 🎯 PROJECT OVERVIEW

### What is Mammuth?

**Mammuth is a compiled systems programming language with universal composition operator.**

**CRITICAL**: The current C++ interpreter is a **PROTOTYPE for language design validation**, NOT the final product!

**Final Goal**: Transpiler to Rust → Native binary compilation

### Key Characteristics

- **Paradigm**: Functional-first with pragmatic imperative features
- **Type System**: Strong, explicit, NO implicit conversions (for safe compilation)
- **Performance**: Transpiles to Rust → LLVM → Native code
- **Philosophy**: "Everything Composes" - Universal `$` operator
- **Target Users**: Systems programmers, CLI tool developers, data pipeline engineers
- **Current Phase**: Prototype interpreter for design validation

### Architecture Vision

```
Phase 1 (NOW): C++ Interpreter
└─> Language design testing
└─> Feature validation
└─> Community feedback

Phase 2 (Q1-Q2 2025): Rust Transpiler
└─> AST → Rust codegen
└─> Type system implementation
└─> Basic standard library

Phase 3 (Q3 2025): Production Compiler
└─> Optimizations
└─> Full standard library
└─> Native binaries
└─> Package manager
```

### Comparison with Other Languages

| Feature | Rust | Go | Zig | **Mammuth** |
|---------|------|----|----|-------------|
| Compiled | ✅ | ✅ | ✅ | ✅ (via Rust) |
| Type safe | ✅ | ✅ | ✅ | ✅ |
| `$` Universal | ❌ | ❌ | ❌ | ✅ ⭐ |
| `??` Condchain | ❌ | ❌ | ❌ | ✅ ⭐ |
| Filter `=>` | ❌ | ❌ | ❌ | ✅ ⭐ |
| Card structures | ❌ | ❌ | ❌ | ✅ ⭐ |
| Expression-oriented | Partial | ❌ | ❌ | ✅ |

**Mammuth is NOT "Rust clone"** - it has unique identity!

---

## 💭 LANGUAGE PHILOSOPHY

### Core Principles

#### 1. **Universal Composition** 🌟

**Everything composes through the `$` operator:**

```mammuth
# Functions compose
pipeline = transform $ filter $ map

# Strings concatenate
message = "Hello" $ " " $ "World"

# Arrays concatenate
combined = [1, 2] $ [3, 4] $ [5]
```

**Philosophy**: One operator, multiple types, consistent behavior!

**Rules**:
- ✅ Same types required (type-safe!)
- ✅ String $ String → String
- ✅ Array $ Array → Array
- ✅ Function $ Function → Composed function
- ❌ String $ Int → ERROR (must use `str()`)
- ❌ Int $ Int → ERROR (use `+` for math)

---

#### 2. **Type Safety for Compilation** 🔒

**NO implicit conversions - everything explicit:**

```mammuth
# ❌ ERRORS (by design!)
"text" $ 42              # Different types
int x = 3.14             # No auto-cast
1 $ 2                    # Ambiguous (concatenate or add?)

# ✅ CORRECT (explicit)
"text" $ str(42)         # "text42"
int x = toInt(3.14)      # 3
str(1) $ str(2)          # "12"
```

**Why**: Safe code generation for compiler!

---

#### 3. **Expression-Oriented** 🎯

**Everything returns a value:**

```mammuth
# If returns value
int max = if a > b:: a else:: b

# Functions return last expression
def double(x: int) -> int::
    x * 2  # Implicit return
end

# Condchain returns value
string grade = score >= 90 ? "A" ?? score >= 80 ? "B" : "C"
```

**Why**: Composable, functional, elegant!

---

#### 4. **Consistent Syntax** 📐

**`::` = Block start, `:` = Inline/type annotation, `.` = Field access**

```mammuth
# :: for blocks
def func():: ... end
if cond:: ... end
card Name:: ... end

# : for inline and types
card Point: int x, int y
int x: 10
def func(x: int): x * 2

# . for fields
p.x
person.name
```

**Philosophy**: Predictable, learnable, no surprises!

---

#### 5. **Pragmatic, Not Pure** ⚖️

**Functional-first, but pragmatic:**

✅ Mutability by Default for Scalars: Standard scalar types (int, double, string, etc.) are mutable by default.

✅ Immutability by Default for Complexes: Complex types, such as arrays and function variables, are immutable by default.

✅ Explicit Immutability (fixed): Scalars can be explicitly made immutable using the fixed keyword.

✅ Explicit Mutability (dynamic): Arrays and function variables can be explicitly made mutable using the dynamic keyword.

✅ Pure or Near-Pure Functions: Functions are pure in the sense that they cannot modify external variables (variables outside their scope).

✅ Side Effects Allowed: Side effects like I/O and printing are fully allowed, ensuring real-world utility.

✅ Imperative Loops: Standard imperative control flow (while, for-in) is available for clear, straightforward logic.

❌ No Purity Dogma: While functions are constrained, there is no strict functional purity dogma that bans all side effects across the entire language.

**Why**: Usable for real-world problems!

---

## ⭐ UNIQUE FEATURES

### What Makes Mammuth Different?

#### 1. 💎 `$` - Universal Composition Operator

**THE defining feature of Mammuth!**

```mammuth
# String concatenation
string msg = "Hello" $ " " $ "World"

# Array concatenation
int[] nums = [1, 2] $ [3, 4] $ [5]

# Function composition
<(int)> pipeline = double $ addTen $ square
# (f $ g $ h)(x) = h(g(f(x))) - left to right!

# Type-safe
"text" $ 42        # ❌ ERROR: types must match!
"text" $ str(42)   # ✅ OK: both strings
```

**No other language has universal `$`!** 🔥

---

#### 2. ⚡ `??` - Condchain (Chainable Ternary)

**Elegant multi-condition expressions:**

```mammuth
# Instead of nested ternaries:
x = cond1 ? val1 : (cond2 ? val2 : (cond3 ? val3 : default))

# Condchain:
x = cond1 ? val1 ?? cond2 ? val2 ?? cond3 ? val3 : default

# Real example:
int grade = score >= 90 ? 5 
         ?? score >= 80 ? 4 
         ?? score >= 70 ? 3 
         ?? score >= 60 ? 2 
         : 1

# With side effects:
(user.isAdmin) ? grantFullAccess() 
              ?? (user.isPremium) ? grantPremiumAccess() 
              : grantBasicAccess()
```

**More readable than nested ternaries!** ✨

---

#### 3. 🎴 Card - Simple Structures

**Data structures with clear syntax:**

```mammuth
# Inline definition
card Point: int x, int y
card Person: string name, int age

# Multiline definition
card Employee::
    string name
    int age
    double salary
    <(string)>->int validator  # Function type in card!
end

# Literal creation
Point p: x=10, y=20
Person mario: name="Mario", age=30

# Field access with .
echo str(p.x)      # 10
p.y = 30           # Modify

# Function parameters
def distance(Point: x=x1, y=y1, Point: x=x2, y=y2) -> double::
    # calculate distance
end

# Function returns
def origin() -> Point::
    Point: x=0, y=0
end
```

**Simpler than Rust structs, clearer than Go!** 🎯

---

#### 4. 🔀 Filter `=>` (Planned v3.6)

**Array operations with elegant syntax:**

```mammuth
# Filtering
int[] evens = numbers => (x: x % 2 == 0)

# Transforming
int[] doubled = numbers => (x: x * 2)

# Chaining
result = arr => filterPositive => double => sum

# With composition
pipeline = filterEven $ double $ sum
```

**Pipeline elegance for data processing!** 🚀

---

#### 5. 🎭 Mask Operators (Planned v3.7)

**Advanced filtering with pattern matching:**

```mammoth
# Syntax TBD - advanced array operations
# More powerful than simple filter
```

---

#### 6. 🔧 Function Types Complete

**Full function signatures required:**

```mammuth
# ALWAYS with return type!
<(int)>->zero        # void function
<(int)>->int         # returns int
<(string, int)>->double  # multiple params

# First-class values
<(int)> transform = if x > 5:: double else:: identity

# In card structures
card Handler: <(int)>->zero callback, string name

# In parameters
def apply(int x, <(int)>->int func) -> int::
    func(x)
end

# ❌ WRONG - incomplete signature
card X: <(int)> func  # Missing return type!
```

**No ambiguity - everything explicit!** ✅

---

#### 7. 🎯 If as Expression

**If returns value, composable:**

```mammuth
# Inline
int max = if a > b:: a else:: b

# Multiline with side effects
int result = if x > 10::
    echo "processing big value"
    100
elif x > 5::
    50
else::
    10
end

# With composition
<(int)> transform = if value > 3:: double else:: identity
```

**Not just control flow - it's an expression!** 💡

---

## ✅ CURRENT STATUS

### Implemented Features (100% Working)

#### **Core Type System** ✅
```mammuth
int x = 10
double y = 3.14
string s = "hello"
int[] arr = 1, 2, 3
<(int)>->int func = double

# Mutability
fixed int immutable = 10     # Default
dynamic int mutable = 20     # Explicit mutable

# Functions ALWAYS fixed (immutable)
```

---

#### **`$` Universal Composition** ✅
```mammoth
# Strings
string msg = "Hello" $ " " $ "World"

# Arrays
int[] combined = [1, 2] $ [3, 4]

# Functions
<(int)> pipeline = double $ addTen $ square
echo str(pipeline(5))  # square(addTen(double(5)))

# Type safety
1 $ 2                # ❌ ERROR: use str(1) $ str(2)
"text" $ 42          # ❌ ERROR: use "text" $ str(42)
```

**Order**: Left-to-right application: `f $ g` = apply f, then g

---

#### **Slice Concatenation** ✅
```mammoth
# Syntactic sugar: var $ [slice] = var $ var[slice]

# String slice concat
"hello" $ [2..]      # "hello" $ "hello"[2..] = "hellollo"
"hello" $ [1..3]     # "hello" $ "hello"[1..3] = "helloell"

# Array slice concat
int[] arr = 1, 2, 3, 4, 5
arr $ [2..4]         # arr $ arr[2..4] = [1,2,3,4,5,3,4,5]
arr $ [0]            # arr $ arr[0] = [1,2,3,4,5,1]

# Useful for string/array duplication patterns
string repeated = str $ [0..len(str)]
```

**Semantics**: Always concatenates! Not just slicing.

---

#### **`??` Condchain** ✅
```mammuth
int grade = score >= 90 ? 5 
         ?? score >= 80 ? 4 
         ?? score >= 70 ? 3 
         : 0

string size = x > 10 ? "big" 
           ?? x > 5 ? "medium" 
           : "small"
```

---

#### **If/Elif/Else Expression** ✅
```mammuth
# Inline
int max = if a > b:: a else:: b

# Multiline
int result = if x > 10::
    echo "big value"
    100
elif x > 5::
    50
else::
    10
end

# Nested
int nested = if a > 5::
    if b > 10:: 100 else:: 50 end
else::
    10
end

# With composition
<(int)> transform = if value > 3:: double else:: identity
echo str(transform(5))  # Calls the returned function!
```

**Status**: 9/9 tests passing! ✅

---

#### **Random Functions** ✅
```mammoth
# Random integer [min, max)
int dice = randInt(1, 7)  # 1-6 inclusive

# Random double [0.0, 1.0)
double prob = randDouble()

# Automatic seeding with time()
# Range validation built-in
```

---

#### **Function Values** ✅
```mammoth
# Variables can hold functions
<(int)> transform = double

# If can return functions
<(int)> func = if x > 5:: double else:: addTen

# Call function variables
int result = transform(10)
```

---

#### **Closures Complete** ✅
```mammuth
# Functions returning functions
def makeAdder(x: int) -> <(int)>->int::
    <(int)> adder: (y: int): x + y  # Captures x!
end

<(int)> add5 = makeAdder(5)
echo str(add5(10))  # 15

# Nested functions with closure
def outer(x: int) -> int::
    def inner(y: int) -> int::
        x + y  # Access outer's x
    end
    inner(10)
end
```

**Automatic scope capture!** 🔐

---

#### **Multi-line Comments** ✅
```mammuth
# Single line comment

#[
  Multi-line comment
  Block comment
  Everything here is ignored
]#

def func()::
    #[
      Function documentation
      Can be multiple lines
    ]#
    42
end
```

---

#### **Built-in Functions** ✅
```mammuth
# Conversion
string s = str(123)
int i = toInt(3.14)
double d = toDouble(123)

# Array operations
int len = len(arr)
array_push(arr, value)  # Dynamic arrays only
array_pop(arr)

# String operations
int length = len(str)

# I/O
string input = input("Enter name: ")
echo "Hello " $ name
err("Error message")

# Random
int dice = randInt(1, 7)
double prob = randDouble()
```

---

#### **Control Flow** ✅
```mammuth
# While with return
int sum = 0
while sum < 10 -> sum::
    sum = sum + 1
end

# For-in
int total = 0
for x in range(1, 11) -> total::
    total = total + x
end

# Range
int[] nums = range(1, 10)  # [1,2,3,4,5,6,7,8,9]
```

---

#### **Operators** ✅
```mammuth
# Arithmetic
+ - * / %

# Comparison
== != < > <= >=

# Logical
and or not

# Bitwise
& | ^ ~ << >>

# Universal composition
$

# Ternary/Condchain
? : ??
```

---


####  Filter `=>` **
```mammuth
# Filtering
int[] evens = numbers => (x: x % 2 == 0)

# Transform
int[] doubled = numbers => (x: x * 2)

# Chain
result = arr => filter => transform => reduce

# Variable name ALWAYS `x` (consistency!)
```



---


## 🚧 PLANNED FEATURES (NOT YET IMPLEMENTED)

### Priority Timeline


####  Mask Operators**
```mammuth
# Advanced filtering with patterns
# Syntax TBD - more powerful than simple filter
```


#### Card Structures**
```mammuth
# Inline
card Point: int x, int y

# Multiline
card Employee::
    string name
    int age
    double salary
    <(string)>->int validator
end

# Literals
Point p: x=10, y=20

# Access
echo str(p.x)
p.y = 30

# In functions
def distance(Point: x=x1, y=y1, Point: x=x2, y=y2) -> double::
    # calculate
end
```

---

#### Rust Transpiler**
```
Phase 1: Parser Rust (from scratch or port)
Phase 2: AST → Rust codegen
Phase 3: Type system implementation
Phase 4: Standard library mapping
Phase 5: Testing & optimization
```


---

## 📖 SYNTAX REFERENCE COMPLETE

### Variables
```mammuth
# Declaration with initialization
int x = 10
double y = 3.14
string s = "hello"

# Arrays
int[] arr = 1, 2, 3
string[] words = "a", "b", "c"

# Dynamic (mutable)
dynamic int counter = 0
dynamic int[] list

# Fixed (immutable - default)
fixed int constant = 42

# Functions (always fixed!)
<(int)>->int func = double
```

---

### Functions
```mammuth
# Named function
def add(x: int, y: int) -> int::
    x + y
end

# Lambda (inline)
<(int, int)>->int add = def(x: int, y: int) -> int: x + y

# Returning function
def makeAdder(n: int) -> <(int)>->int::
    <(int)> adder: (x: int): x + n
end

# Nested functions
def outer() -> int::
    def inner() -> int::
        42
    end
    inner()
end

# Higher-order
def apply(x: int, f: <(int)>->int) -> int::
    f(x)
end
```

---

### Control Flow
```mammuth
# Condchain
int x = cond ? val1 ?? cond2 ? val2 : default

# If/Else expression
int result = if cond::
    value1
elif cond2::
    value2
else::
    value3
end

# If inline
int max = if a > b:: a else:: b

# While with return
int sum = 0
while sum < 10 -> sum::
    sum = sum + 1
end

# For-in with return
int total = 0
for x in range(10) -> total::
    total = total + x
end
```

---

### Operators
```mammuth
# Arithmetic
x + y    # Addition
x - y    # Subtraction
x * y    # Multiplication
x / y    # Division
x % y    # Modulo

# Comparison
x == y   # Equal
x != y   # Not equal
x < y    # Less than
x > y    # Greater than
x <= y   # Less or equal
x >= y   # Greater or equal

# Logical
x and y  # Logical AND
x or y   # Logical OR
not x    # Logical NOT

# Bitwise
x & y    # Bitwise AND
x | y    # Bitwise OR
x ^ y    # Bitwise XOR
~x       # Bitwise NOT
x << n   # Left shift
x >> n   # Right shift

# Universal Composition
x $ y    # String/Array concat, Function compose

# Slice Concatenation (sugar for x $ x[slice])
x $ [start..end]   # x $ x[start..end]
x $ [start..]      # x $ x[start..]
x $ [..end]        # x $ x[..end]

# Ternary/Condchain
cond ? val1 : val2              # Simple ternary
c1 ? v1 ?? c2 ? v2 : default    # Condchain
```

---

### Comments
```mammuth
# Single-line comment

#[
  Multi-line comment
  Can span multiple lines
  Nested: #[ inner ]#
]#
```

---

### Card
```mammuth
# Inline definition
card Point: int x, int y
card Person: string name, int age

# Multiline definition
card Employee::
    string name
    int age
    double salary
    <(string)>->int validator
end

# Literal creation
Point p: x=10, y=20

# Field access
echo str(p.x)
p.y = 30  # If dynamic

# Function params
def distance(Point: x=x1, y=y1, Point: x=x2, y=y2) -> double

# Function return
def origin() -> Point::
    Point: x=0, y=0
end
```

---

### Filter
```mammuth
# Filter array
int[] evens = numbers => (x: x % 2 == 0)

# Transform array
int[] doubled = numbers => (x: x * 2)

# Chain operations
result = arr => filterPositive => double => sum
```

---



---

🦣 **MAMMUTH - Where Everything Composes** 🦣


