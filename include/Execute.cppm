module;

#include <bsu.hpp>

export module bsu:Execute;

import std;

export namespace bsu {
    BSU_EXPORT auto execute(const std::string& command, std::span<const std::string> customEnv = {}) -> std::expected<int, std::string>;
}
