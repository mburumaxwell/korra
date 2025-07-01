# DHTesp

I copied this from <https://github.com/beegee-tokyo/DHTesp/tree/b27100ac25cfc02244eaaa0f6b8ee8a225b37896> just to do the following modifications:

1. Ensure `pinMode(...)` with the correct argument is called before `digitalWrite(...)` or `digitalRead(...)`.

> The code looks more different because it was forced to follow clang-rules in this repo.
