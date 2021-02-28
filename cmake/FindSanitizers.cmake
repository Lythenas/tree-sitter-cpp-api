# Source: http://www.stablecoder.ca/2018/02/01/analyzer-build-types.html

# use CMAKE_BUILD_TYPE to select flags to use

# detects e.g. out-of-bounds access, use-after-free, double-free, memory leaks
set(CMAKE_CXX_FLAGS_ASAN
    "-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
    CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds."
    FORCE)

# detects uninitialized reads (may have false positives)
# does not work with gcc only clang
set(CMAKE_CXX_FLAGS_MSAN
    "-fsanitize=memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fsanitize-memory-use-after-dtor -fno-omit-frame-pointer -g -O1"
    CACHE STRING "Flags used by the C++ compiler during MemorySanitizer builds."
    FORCE)

# detects uses of undefined behaviour (e.g. nullptr dereferencing, accessing unaligned memory)
set(CMAKE_CXX_FLAGS_UBSAN
    "-fsanitize=undefined"
    CACHE STRING "Flags used by the C++ compiler during UndefinedBehaviourSanitizer builds."
    FORCE)
