## ARCHITECTURE

## Language standered
- c++ 26

## Design
I try to use the most modern aproche to programing like:
- Data Orented Design

I also try to precompile the big header to cashe for better
compile times in pch.h that a precompile definition in CMakeList.txt.

## Coding style
| Category       | Example               | Convention  |
|----------------|-----------------------|-------------|
| Data Types     | structs, class        | Pascal_Case |
| Varables       | uint, float           | snake_case  |
| Namespace      | n_swapchain           | n_prefix    | 
| Member         | m_device              | m_prefix    |

I use #pragma once just cleaner as the #ifndef, #define, #endif
stuff is just old code.
