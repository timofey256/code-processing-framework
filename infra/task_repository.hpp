#pragma once

#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <expected>

#include "core/types.hpp"

class task_repository {
public:
    std::string add(task t); 
    std::expected<task, std::string> get(std::string& task_id); 
    void change_status(std::string& id, task_status status); 
    std::expected<std::string, std::string> remove(std::string& task_id); 
    bool contains(std::string& task_id); 
    void clear();
private:
    std::unordered_map<std::string, task> tasks;
};
