module;

#include <bsu.hpp>

export module bsu.filesystem;

import std;

export namespace bsu::filesystem {
    BSU_EXPORT auto move_to_trash(const std::filesystem::path& path) -> bool;
}
