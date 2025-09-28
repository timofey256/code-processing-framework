#pragma once

#include <optional>
#include <string>
#include <cstdlib>
#include <iostream>

std::optional<std::string> get_env(const char* name);
std::optional<int> get_port(const char* env_name);
std::string get_pg_conninfo();
