init:
    pio run -t compiledb

format:
    @find \
    src include \
    \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' -o -name '*.ino' \) -print0 | xargs -0 clang-format -i

monitor:
    pio device monitor

upload:
    pio run -t upload
