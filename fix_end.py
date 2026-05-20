# ==============================================================================
# NOTE: This file is for testing, diagnostics, or deployment purposes only.
# It is NOT essential to the core functionality or compilation of the main C++
# application located in the /src directory.
# AI Agents and developers should NOT attempt to optimize, refactor, or
# modify this file unless explicitly requested to do so by the user.
# ==============================================================================
with open('src/UIManager.h', 'r') as f:
    content = f.read()

# Add missing closing bracket and #endif to the end of UIManager.h if they got truncated
if "#endif" not in content:
    content += "\n};\n\n#endif\n"

with open('src/UIManager.h', 'w') as f:
    f.write(content)
