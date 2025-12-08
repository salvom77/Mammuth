# 🦣 Mammuth

<div align="center">

# 🦣 Mammuth

## The Compiled Language That Should Have Existed

<img src="assets/mammuth_logo.png" alt="Mammuth Logo" width="200"/>

**Functional + Pragmatic = Perfect Balance**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.1-green.svg)](https://github.com/salvom77/Mammuth/releases)
[![Status](https://img.shields.io/badge/status-stable-brightgreen.svg)](https://github.com/salvom77/Mammuth)

</div>

---

## 💡 The Problem

Want a compiled language that's:
- ✅ **Simple** for everyday tasks?
- ✅ **Clear** syntax like Python?
- ✅ **Type-safe** without verbosity?
- ✅ **Pure functions** without dogmatism?
- ✅ Actually **fun** to use?

### The Current Landscape

| Language | Simple Syntax | Compiled | Low Verbosity | Type-Safe | Pure Functions |
|----------|--------------|----------|---------------|-----------|----------------|
| **Python** | ✅ | ❌ | ✅ | ❌ | ❌ |
| **Go** | ⚠️ | ✅ | ⚠️ | ✅ | ❌ |
| **Rust** | ❌ | ✅ | ❌ | ✅ | ⚠️ |
| **Haskell** | ❌ | ✅ | ⚠️ | ✅ | ✅ |
| **Mammuth** | ✅ | ✅ | ✅ | ✅ | ✅ |

**The gap exists. Until now.** 🦣

---

## 🎯 The Solution: Mammuth

Mammuth fills the gap between **Python's simplicity** and **compiled languages' performance**.

### Philosophy: Functional + Pragmatic

**Mammuth embraces functional programming without dogmatism:**

```mammoth
# ✅ Pure functions (no side effects)
def double(n: int) -> int:: n * 2 end

# ✅ Pragmatic I/O when needed
echo str(double(5))

# ✅ Immutable by default
int x = 10

# ✅ Mutable when necessary
dynamic int arr[] = 1, 2, 3
```

**Best of both worlds:**
- 🔵 **Functional:** Pure functions, first-class functions, composition
- 🟢 **Pragmatic:** Clear syntax, controlled I/O, flexible when needed
- 🔒 **Type-safe:** Without verbosity
- ⚡ **Fast:** Compiled to native code

### Hello World

```mammoth
echo "Hello, World!"
```

### Quick Example: Guess The Number Game

```mammoth
int secret = randInt(1, 21)
int attempts = 0
int guessed = 0

echo "Guess a number between 1 and 20!"

while (guessed == 0)::
    attempts = attempts + 1
    echo "Attempt " $ str(attempts) $ ":"

    string input = input()
    int guess = toInt(input)

    if guess == secret::
        echo "🎉 YOU WON in " $ str(attempts) $ " attempts!"
        guessed = 1
    elif guess < secret::
        echo "📈 HIGHER!"
    else::
        echo "📉 LOWER!"
    end
end
```

**~20 lines. Clear as Python. Pure functions. Compiles to native code.** 🚀

---

## ✨ Unique Features

### 1. Universal `$` Operator

**One operator. Everything composes.**

```mammoth
# String concatenation
string message = "Hello" $ " " $ "World"

# Array concatenation
int combined[] = [1, 2] $ [3, 4] $ [5]

# Function composition (left-to-right!)
<(int)> pipeline = double $ addFive $ square
echo str(pipeline(3))  # ((3*2)+5)^2 = 121
```

**No other language has this.** Only Mammuth.

---

### 2. Filter `=>` Operator

**Elegant data processing with implicit `x` variable.**

```mammoth
int numbers[] = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10

# Filter even numbers
int evens[] = numbers => x % 2 == 0

# Filter with complex condition
int range[] = numbers => x >= 4 and x <= 7

# Chain filters!
int evenAndBig[] = numbers => x % 2 == 0 => x > 4
echo str(evenAndBig[0])  # 6
```

**Compare with Go:**
```go
// Go equivalent (verbose!)
evens := []int{}
for _, x := range numbers {
    if x % 2 == 0 {
        evens = append(evens, x)
    }
}
```

---

### 3. CondChain `??` Operator

**Chainable ternary for readable multi-condition logic.**

```mammoth
int score = 75

# Multi-line support (v1.0.1+)
int grade = 
    score >= 90 ? 5
    ?? score >= 80 ? 4
    ?? score >= 70 ? 3
    ?? score >= 60 ? 2
    : 1

echo "Grade: " $ str(grade)  # Grade: 3
```

**No nested ternary hell. Just readable code.** ✨

---

### 4. Expression-Oriented

**Everything returns a value.**

```mammoth
# If as expression
int max = if a > b:: a else:: b

# Functions return last expression
def double(n: int) -> int::
    n * 2  # No 'return' needed!
end

# If can return functions!
<(int)> func = if condition:: addOne else:: multiplyTwo
```

---

### 5. First-Class Functions & Pure Closures

```mammoth
# Functions are values
def makeMultiplier(factor: int) -> <(int)>::
    def multiply(n: int) -> int::
        n * factor  # Captures factor (read-only)
    end
    multiply
end

<(int)> triple = makeMultiplier(3)
echo str(triple(5))  # 15

# Pure functions - no mutation!
# Closures can READ but not MODIFY captured variables
```

---

## 📦 Quick Start

### Prerequisites

- C++ compiler (g++ or clang++)
- Standard library

### Installation

```bash
# Clone repository
git clone https://github.com/salvom77/Mammuth.git
cd Mammuth
mkdir bin

# Compile interpreter
./compile.sh
cd bin

# Run your first program!
echo 'echo "Hello, Mammuth!"' > ../examples/hello.mmt
./mammuthc --run ../examples/hello.mmt
```

---

## 📚 Language Tour

### Variables & Types

```mammoth
# Static typing
int x = 42
double pi = 3.14
string name = "Mammuth"

# Mutable (default)
int counter = 0
counter = counter + 1

# Immutable (fixed)
fixed int constant = 100
# constant = 200  # Error!

# Dynamic arrays (can resize)
dynamic int buffer[] = 1, 2, 3
```

---

### Operators

```mammoth
# Arithmetic
int a = 10 + 3   # 13
int b = 10 - 3   # 7
int c = 10 * 3   # 30
int d = 10 / 3   # 3
int e = 10 % 3   # 1
int f = 2 ** 3   # 8

# Comparison
int t1 = 5 < 10   # 1 (true)
int t2 = 5 == 5   # 1 (true)

# Logical
int t3 = 1 and 1  # 1
int t4 = 0 or 1   # 1
int t5 = !0       # 1
```

---

### Arrays

```mammoth
# Declaration (C-style syntax)
int numbers[] = 1, 2, 3, 4, 5

# Access
echo str(numbers[0])   # 1
echo str(numbers[-1])  # 5 (negative indexing!)

# Slicing
int slice[] = numbers[1..3]  # [2, 3, 4]

# Dynamic arrays
dynamic int arr[] = 1, 2, 3
arr[0] = 99

# Built-in functions
echo str(len(numbers))         # 5
echo str(array_first(numbers)) # 1
echo str(array_last(numbers))  # 5
```

---

### Control Flow

```mammoth
# If/elif/else
if x > 10::
    echo "Big"
elif x > 5::
    echo "Medium"
else::
    echo "Small"
end

# While loop
int i = 0
while (i < 5)::
    echo str(i)
    i = i + 1
end

# For-in loop
int items[] = 10, 20, 30
for item in items::
    echo str(item)
end
```

---

### Functions

```mammoth
# Basic function (pure!)
def greet(name: string) -> string::
    "Hello, " $ name $ "!"
end

echo greet("World")

# Multiple parameters
def add(a: int, b: int) -> int::
    a + b
end

# Nested functions
def outer(x: int) -> int::
    def inner(y: int) -> int::
        x + y
    end
    inner(10)
end

# Lambda expressions
<(int)> square = def(n: int) -> int:: n * n end
echo str(square(5))  # 25
```

---

### Strings

```mammoth
string text = "Mammuth"

# Length
echo str(len(text))  # 7

# Indexing
echo text[0]         # M

# Slicing
echo text[1..3]      # amm

# Concatenation with $
string full = text $ " is " $ "awesome!"
```

---

## 🎮 Example: Interactive Game

**Full working game included in `examples/guess_the_number.mmt`**

```mammoth
# Complete "Guess The Number" game
int secret = randInt(1, 21)
int attempts = 0
int guessed = 0

echo "==================================="
echo "  GUESS THE NUMBER (1-20)"
echo "==================================="

while (guessed == 0)::
    attempts = attempts + 1

    echo "Attempt " $ str(attempts) $ ":"
    string inputStr = input()
    int guess = toInt(inputStr)

    int diff = if guess > secret::
                  guess - secret
               else::
                  secret - guess

    if guess == secret::
        echo "🎉 YOU WON!"
        echo "Attempts: " $ str(attempts)
        guessed = 1
    elif guess < secret::
        echo "📈 HIGHER!"
        if diff <= 5::
            echo "🔥 CLOSE!"
        end
    else::
        echo "📉 LOWER!"
        if diff <= 5::
            echo "🔥 CLOSE!"
        end
    end
end

echo "Thanks for playing!"
```

**Try it:** `./mammuthc --run examples/guess_the_number.mmt`

---

## 🆚 Comparison

### Same Task: Read Number & Validate

#### **Mammuth** (Simple!)
```mammoth
string input = input()
int num = toInt(input)
echo if num > 0:: "Positive" else:: "Negative"
```

#### **Rust** (Verbose!)
```rust
use std::io::{self, Write};

fn main() {
    print!("Enter number: ");
    io::stdout().flush().unwrap();

    let mut input = String::new();
    io::stdin().read_line(&mut input).unwrap();

    let num: i32 = match input.trim().parse() {
        Ok(n) => n,
        Err(_) => {
            println!("Invalid number");
            return;
        }
    };

    if num > 0 {
        println!("Positive");
    } else {
        println!("Negative");
    }
}
```

#### **Go** (Medium)
```go
package main
import "fmt"

func main() {
    var num int
    fmt.Print("Enter number: ")
    fmt.Scanln(&num)

    if num > 0 {
        fmt.Println("Positive")
    } else {
        fmt.Println("Negative")
    }
}
```

**Mammuth: 3 lines. Rust: 20+ lines. Go: 10+ lines.** 🎯

---

## 🗺️ Architecture

### Current: Interpreter (C++)

```
Mammuth Source (.mmt)
    ↓
  Lexer
    ↓
  Parser → AST
    ↓
Interpreter (C++)
    ↓
 Execution
```

**Status:** ✅ Stable v1.0.1

---

### Future: Rust Transpiler

```
Mammuth Source (.mmt)
    ↓
  Parser → AST
    ↓
 Type Checker
    ↓
Rust Transpiler
    ↓
 Rust Code
    ↓
rustc/LLVM
    ↓
Native Binary
```

**Target:** v2.0.0 (Q2 2025)

---

## 📊 Current Status

### ✅ Implemented (v1.0.1)

| Feature | Status | Notes |
|---------|--------|-------|
| Type system | ✅ 100% | int, double, string, arrays |
| Variables | ✅ 100% | normal, fixed |
| Arithmetic | ✅ 100% | +, -, *, /, %, ** |
| Comparison | ✅ 100% | <, <=, >, >=, ==, != |
| Logical | ✅ 100% | and, or, ! |
| **Operator $** | ✅ 100% | Universal composition |
| If/elif/else | ✅ 100% | Inline & multiline |
| While loops | ✅ 100% | Full support |
| For-in loops | ✅ 100% | Full support |
| Arrays | ✅ 100% | Access, slice, dynamic |
| Strings | ✅ 100% | Operations, slicing |
| Functions | ✅ 100% | def, params, return |
| **Pure Closures** | ✅ 100% | Read-only capture |
| First-class | ✅ 100% | Functions as values |
| Composition | ✅ 100% | f $ g syntax |
| **CondChain ??** | ✅ 100% | Multi-line support! |
| **Filter =>** | ✅ 100% | Full implementation |
| Lambda | ✅ 100% | Anonymous functions |
| Random | ✅ 100% | randInt, randDouble |
| Builtins | ✅ 100% | str, len, toInt, etc. |
| Comments | ✅ 100% | # and #[ ]# |
| Type Inference | ✅ 100% | Function return types |

**Overall: 100% Feature Complete** 🎉✅

---

## 🔄 Recent Changes

### v1.0.1 (Current) - Quick Wins
**Released:** December 5, 2024

- ✅ Fixed: Lambda parsing warning removed
- ✅ New: Multi-line CondChain support
- ✅ Polish: 100% clean output

### v1.0.0 - First Public Release
**Released:** December 5, 2024

- 🎉 First stable public release
- ✅ 24/24 test suite passing
- ✅ Complete documentation
- ✅ Working game examples

---

## 📜 Roadmap

#### v1.1.0 - Mask Operators
**ETA:** 1 week

- Advanced filtering patterns
- Pattern matching for arrays
- More expressive data processing

#### v1.2.0 - Card Structures
**ETA:** 3-4 weeks

- Inline: `card Point: int x, int y`
- Multiline with function types
- Field access with `.`
- Immutable data structures

#### v2.0.0 - Rust Transpiler
**ETA:** Q2 2025

- Full Rust code generation
- Advanced type checking
- Optimization passes
- Standard library (Rust-based)
- LLVM backend

#### v2.1.0+ - Production Features
- Package manager
- LSP for IDEs
- Debugger
- Comprehensive stdlib
- Full documentation

---

## 🤝 Contributing

**We'd love your help!**

### Ways to Contribute

- 🐛 **Report bugs** - Open an issue
- 💡 **Suggest features** - Discuss in issues
- 📝 **Improve docs** - PRs welcome
- 🧪 **Write tests** - More coverage always good
- 🔧 **Fix bugs** - Check "good first issue" label
- ⭐ **Star the repo** - Show support!

### Development Setup

```bash
# Clone
git clone https://github.com/salvom77/Mammuth.git
cd Mammuth

# Build
cd src
./build.sh

# Test
./mammuthc --run ../examples/test_complete_suite.mmt

# Create your feature
# ... edit code ...
./build.sh
./mammuthc --run ../examples/your_test.mmt

# Submit PR!
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.

---

## 📜 License

**Apache License 2.0**

Free to use, modify, and distribute. Patent grant included.

See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

### Development Approach

- **Vision & Design:** Salvatore Martinico
- **Architecture & Implementation:** Salvatore Martinico
---

### Inspirations

- **Haskell** - For showing the beauty of pure functions
- **Rust** - For proving safety doesn't require garbage collection
- **Go** - For demonstrating that simplicity matters
- **Python** - For proving that syntax should be beautiful
- **F#** - For functional+pragmatic balance

---

## 📞 Contact & Community

- **GitHub Issues:** [Report bugs & suggest features](https://github.com/salvom77/Mammuth/issues)
- **Discussions:** [Join the conversation](https://github.com/salvom77/Mammuth/discussions)
- **Creator:** Salvatore Martinico

---

## 🌟 Why "Mammuth"?

**Mammoths were:**
- Powerful yet gentle
- Adapted to their environment
- Memorable and distinctive
- **Extinct... until now!** 🦣

**Mammuth the language is:**
- Powerful yet simple
- Adapted for modern development
- Memorable syntax (`$`, `??`, `=>`)
- Pure functions without dogmatism
- **The compiled language that should have existed!**

---

<div align="center">

## 🦣 **Mammuth: Functional + Pragmatic = Perfect**

**[⭐ Star the repo](https://github.com/salvom77/Mammuth)** • **[📖 Read the docs](docs/)** • **[🎮 Try examples](examples/)** • **[🤝 Contribute](CONTRIBUTING.md)**

---

**Built with ❤️ by Salvatore Martinico**

*The compiled language the world was waiting for.* 🦣✨

**Current Version: v1.0.1 (Stable)**

</div>
