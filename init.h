#pragma once

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x "=" PRINT_MACRO_HELPER(x)

#define IS_DEBUG (1)
// #define IS_DEBUG (0)

#define BUFFERSIZE 4096