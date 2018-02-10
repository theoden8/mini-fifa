#pragma once

#if defined(__clang__) || defined(__GNUG__)
  #define ATTRIB_PACKED __attribute__((packed))
#else
  #define ATTRIB_PACKED
#endif
