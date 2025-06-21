#include "Environment.hpp"

#include <cassert>

std::filesystem::path get_system_home_path()
{
    auto home = getenv("HOME");
    assert(home != nullptr && "Why are you homeless?");
    return { home };
}

std::filesystem::path get_application_config_path()
{
#ifdef DEBUG
    return std::filesystem::path(PROJECT_SOURCE_DIR) / "build" / "debug";
#else
    auto configHome = getenv("XDG_CONFIG_HOME");
    if (configHome) return std::filesystem::path(configHome) / PROJECT_NAME;
    return get_system_home_path() / ".config" / PROJECT_NAME;
#endif
}

std::filesystem::path get_application_data_path()
{
#if DEBUG
    return std::filesystem::path(PROJECT_SOURCE_DIR) / "resources";
#else
    auto dataHome = getenv("XDG_DATA_HOME");
    if (dataHome) return std::filesystem::path(dataHome) / PROJECT_NAME;
    return get_system_home_path() / ".local" / "share" / PROJECT_NAME;
#endif
}

