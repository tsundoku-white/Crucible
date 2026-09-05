## **CODE_STYLE**

## Files and Headers
| CONTEXT          | STYLE            | EXAMPLE                                         |
|:-----------------|:-----------------|:------------------------------------------------|
| header guard     | #pragma once     | first line on any header                        |
| file names       | lower_snake_case | vk_context.cc, vk_context.h                     |
| header files     | .h               | --                                              |
| source files     | .cc              | --                                              |

## Naming
| CONTEXT          | NAME_STYLE       | EXAMPLE                                         |
|:-----------------|:-----------------|:------------------------------------------------|
| containers       | PascalCase       | `struct Context`, `struct QueueFamilyIndices`   |
| functions        | camelCase        | `createSwapchain()`, `isComplete()`             |
| locals           | camelCase        | `messengerCI`, `imageCount`                     |
| members          | m_camelCase     | `m_Device`, `m_Swapchain`                       | 
| globals          | g_PascalCase     | `g_FrameIndex`                                  |
| constants        | SCREAMING_SNAKE  | `MAX_ENTITIES`, `ENABLE_DEBUG_DRAW`             |
| macros           | SCREAMING_SNAKE  | `ARRAY_COUNT(arr)`                              |
| namespaces       | n_snake_case     | `namespace n_context`                           |
| enum types       | PascalCase       | `enum class RenderMode`                         |
| enum values      | PascalCase       | `RenderMode::Forward`                           |
| template params  | PascalCase       | `template<typename ComponentType>`              |
 
### Rules of thumb 
Any user written code follows the table above any sdk or lib this table does not apply.
