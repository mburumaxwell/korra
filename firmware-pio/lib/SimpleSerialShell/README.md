# Simple Serial Shell

I copied this from <https://github.com/philj404/SimpleSerialShell/blob/14b80b035ddd2d25c9e1543c95e5592478a01a7a> just to do the following modifications:

1. Move `#include <Arduino.h>` from the `.cpp` file to the `.h` file for it to compile.
2. Adding a tokenizer that accepts quoted values. Though this could have been done in the app using `setTokenizer` function.

> The code looks more different because it was forced to follow clang-rules in this repo.
