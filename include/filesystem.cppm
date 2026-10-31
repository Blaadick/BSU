module;

#include <bsu.hpp>

export module bsu:filesystem;

import std;

export namespace bsu::filesystem {
    BSU_EXPORT auto move_to_trash(const std::filesystem::path& path) -> bool;

    BSU_EXPORT auto move_to_trash(const std::filesystem::path& path, std::error_code& errorCode) noexcept -> bool;
}
