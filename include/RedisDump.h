#pragma once
#include <string>
void flush_redis_stream_to_file(const std::string& stream_name, const std::string& output_file);
