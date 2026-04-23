## 2024-04-23 - Optimize bytearray search with .find()
**Learning:** Found custom python `for` loops parsing UART bytearrays for sync bytes. In Python and especially constrained CircuitPython, manual iteration over bytearrays is extremely slow compared to the underlying native C implementations of methods like `bytearray.find()`. Using `.find()` not only runs significantly faster but avoids unnecessary loop control overhead and repeated slicing.
**Action:** Always prefer native string and byte array methods (e.g. `.find()`, `.index()`) for searching sequences over manual loops.
