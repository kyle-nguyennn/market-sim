#include <sw/redis++/redis++.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace sw::redis;
using json = nlohmann::json;

void flush_redis_stream_to_file(const std::string& stream_name, const std::string& output_file) {
    Redis redis("tcp://127.0.0.1:6379");
    std::ofstream file(output_file);
    if (!file) {
        std::cerr << "Error opening " << output_file << " for writing\n";
        return;
    }

    std::string last_id = "0-0";
    while (true) {
        auto result = redis.xread({{stream_name, last_id}}, 100);
        if (result.empty()) break;

        for (const auto& [stream, entries] : result) {
            for (const auto& [id, fields] : entries) {
                last_id = id;
                json j;
                for (const auto& [key, value] : fields) {
                    j[key] = value;
                }
                file << j.dump() << "\n";
            }
        }
    }

    std::cout << "Flushed Redis stream '" << stream_name << "' to " << output_file << "\n";
}
