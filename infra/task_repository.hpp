#pragma once

#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <expected>

#include "core/types.hpp"

class task_repository {
public:
    std::string add(task t); 
    std::expected<task, std::string> get(const std::string& task_id); 
    void change_status(const std::string& id, task_status status); 
    std::expected<std::string, std::string> remove(const std::string& task_id); 
    bool contains(const std::string& task_id); 
    void clear();
private:
    std::unordered_map<std::string, task> tasks;
};
