import re
with open('src/UIManager.h', 'r') as f:
    content = f.read()

# Add missing closing bracket and #endif to the end of UIManager.h if they got truncated
if "#endif" not in content:
    content += "\n};\n\n#endif\n"

with open('src/UIManager.h', 'w') as f:
    f.write(content)
