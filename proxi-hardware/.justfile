init:
    pio run -t compiledb

format:
    @find \
    src include \
    \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' -o -name '*.ino' \) -print0 | xargs -0 clang-format -i

monitor-arduino:
    pio device monitor -b 115200 -e uno_r4_wifi

monitor-esp32:
    pio device monitor -b 115200 -e esp32dev

upload-arduino:
    pio run -t upload -e uno_r4_wifi

upload-esp32:
    pio run -t upload -e esp32dev
