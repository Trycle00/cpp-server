#ifndef TRY_UTIL_H
#define TRY_UTIL_H

#include <stdint.h>
#include <string>
#include <vector>

namespace trycle
{

uint64_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(const std::vector<std::string>& strings, int size, int skip);

std::string Backtrace(const int size, const int skip, const std::string& prefix);

} // namespace trycle

#endif // TRY_UTIL_H