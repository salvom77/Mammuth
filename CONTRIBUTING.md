# Contributing to Mammuth 🦣

Thank you for your interest in contributing to Mammuth! This document provides guidelines and information for contributors.

## 🌟 Ways to Contribute

### 1. 🐛 Report Bugs

Found a bug? Please open an issue with:
- Clear description of the problem
- Minimal code example that reproduces it
- Expected vs actual behavior
- Your environment (OS, compiler version)

### 2. 💡 Suggest Features

Have an idea? Open an issue with:
- Clear description of the feature
- Use cases and examples
- Why it would benefit Mammuth users

### 3. 📝 Improve Documentation

- Fix typos or unclear explanations
- Add examples to existing docs
- Write tutorials or guides
- Translate documentation

### 4. 🧪 Write Tests

- Add test cases for existing features
- Improve test coverage
- Create edge case tests

### 5. 🔧 Fix Bugs

- Look for issues tagged `good first issue`
- Comment on the issue before starting
- Submit a PR with your fix

### 6. ✨ Add Features

- Discuss major features in an issue first
- Follow the coding style
- Add tests for new functionality
- Update documentation

---

## 🚀 Getting Started

### Development Setup

```bash
# Fork and clone the repository
git clone https://github.com/yourusername/mammuth.git
cd mammuth

# Build the interpreter
./compile.sh

# Run tests
cd bin
./mammuthc --run ../examples/test_complete_suite.mmt

# Try the example game
./mammuthc --run ../examples/guess_the_number.mmt
```

### Project Structure

```
mammuth/
├── src/               # C++ interpreter source
│   ├── lexer.h       # Tokenization
│   ├── parser.h      # AST generation
│   ├── interpreter.h # Execution engine
│   └── mammuthc.cpp  # Main entry point
├── examples/         # Example programs
│   ├── test_*.mmt   # Test files
│   └── *.mmt        # Example programs
├── docs/            # Documentation
└── README.md
```

---

## 📋 Development Guidelines

### Code Style

**C++ Code (Interpreter):**
- Use descriptive variable names
- Comment complex logic
- Follow existing code patterns
- Keep functions focused and small

**Mammuth Code (Examples/Tests):**
- Clear variable names
- Comments for non-obvious code
- Follow examples in `test_complete_suite.mmt`

### Commit Messages

Use clear, descriptive commit messages:

```
✅ Good:
- "Add support for modulo operator (%)"
- "Fix parser error with nested arrays"
- "Improve filter operator performance"

❌ Bad:
- "fix bug"
- "update"
- "changes"
```

### Pull Request Process

1. **Fork the repository**
2. **Create a feature branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes:**
   - Write clean code
   - Add tests if applicable
   - Update documentation
4. **Test thoroughly:**
   ```bash
   ./compile.sh
   ./mammuthc --run ../examples/test_complete_suite.mmt
   ```
5. **Commit your changes:**
   ```bash
   git commit -m "Add feature: description"
   ```
6. **Push to your fork:**
   ```bash
   git push origin feature/your-feature-name
   ```
7. **Open a Pull Request:**
   - Clear title and description
   - Reference any related issues
   - Explain what changes and why

---

## 🧪 Testing

### Running Tests

```bash
# Full test suite
./mammuthc --run ../examples/test_complete_suite.mmt

# Specific test file
./mammuthc --run ../examples/test_filter.mmt

# Your own test
echo 'echo "test"' > mytest.mmt
./mammuthc --run mytest.mmt
```

### Writing Tests

Add tests to `examples/test_complete_suite.mmt` or create new test files:

```mammoth
# Test your feature
echo "=== TEST: My Feature ==="

# Your test code here
int result = myFeature(42)
echo "Result: " $ str(result)

# Expected output comment
# Expected: Result: 84
```

---

## 🐛 Debugging Tips

### Enable Debug Output

In `src/debug.h`:
```cpp
#define DEBUG_SCOPE 1       // Show scope operations
#define DEBUG_TOKEN_DUMP 0  // Show all tokens (very verbose!)
```

### Common Issues

**Parser Errors:**
- Check token types in lexer.h
- Verify grammar rules in parser.h
- Use DEBUG_TOKEN_DUMP to see token stream

**Runtime Errors:**
- Enable DEBUG_SCOPE to track variable scoping
- Add echo statements to trace execution
- Check interpreter.h for evaluation logic

---

## 🎯 Priority Areas

### High Priority (v3.6-3.8)

1. **Fix remaining bugs:**
   - For-in loop syntax
   - Multi-line CondChain
   - Function-returning-function parsing

2. **Implement Mask operators (v3.7):**
   - Pattern matching for arrays
   - Advanced filtering

3. **Implement Card structures (v3.8):**
   - Inline and multiline syntax
   - Field access with `.`

### Medium Priority (v4.0)

1. **Rust Transpiler:**
   - AST → Rust code generation
   - Type checking
   - Optimization

2. **Standard Library:**
   - Math functions
   - String utilities
   - File I/O
   - Collections

### Low Priority (v1.0+)

1. **Tooling:**
   - LSP for IDE support
   - Debugger
   - Package manager

2. **Documentation:**
   - Complete language reference
   - Tutorial series
   - Best practices guide

---

## 💬 Communication

### Getting Help

- **Issues:** Ask questions in GitHub issues
- **Discussions:** Join GitHub discussions
- **Email:** Contact maintainer (see README)

### Reporting Security Issues

Please email security issues directly to the maintainer rather than opening public issues.

---

## 📜 Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inclusive experience for everyone.

### Standards

**Positive behavior:**
- Using welcoming and inclusive language
- Being respectful of differing viewpoints
- Accepting constructive criticism gracefully
- Focusing on what's best for the community

**Unacceptable behavior:**
- Harassment or discriminatory language
- Trolling or insulting comments
- Personal or political attacks
- Publishing others' private information

### Enforcement

Violations may result in temporary or permanent ban from the project.

---

## 🏆 Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes for significant contributions
- Special thanks in README for major features

---

## 📝 License

By contributing to Mammuth, you agree that your contributions will be licensed under the Apache License 2.0.

---

## 🙏 Thank You!

Every contribution, no matter how small, helps make Mammuth better!

**Together we're building the compiled language that should have existed.** 🦣✨

---

**Questions?** Open an issue or start a discussion! We're here to help! 😊
