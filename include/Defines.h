#ifndef PA_DEFINES_H
#define PA_DEFINES_H

#ifndef NDEBUG
#include <fstream>
#include <iostream>
#include <stdexcept>
#endif //NDEBUG

#ifndef NDEBUG
#  define FILE_TRACE_LOG(msg) \
    do { \
      std::ofstream traceFile("trace.log", std::ios::app); \
      if(!traceFile.is_open()) { throw std::runtime_error("Unable to write to debug trace log!"); } \
      traceFile << msg << std::endl; \
    } while(false)
#else
#  define FILE_TRACE_LOG(msg) do{}while(false)
#endif // NDEBUG

#endif
