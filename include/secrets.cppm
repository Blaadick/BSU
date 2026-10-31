module;

#include <bsu.hpp>

export module bsu.secrets;

import std;

export namespace bsu::secrets {
    BSU_EXPORT auto get(const std::string& service, const std::string& name) -> std::expected<std::string, std::string>;

    BSU_EXPORT auto set(const std::string& service, const std::string& name, const std::string& value) -> std::expected<void, std::string>;
}
