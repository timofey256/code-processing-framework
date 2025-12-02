#pragma once

#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <expected>
#include <pqxx/pqxx>

#include "core/types.hpp"

class task_repository {
public:
    task_repository(pqxx::connection& c) : conn(c) {}

    std::string add_submission(task_submission t); 
    std::string save_result(task_result t); 

    bool contains_result(std::string task_id);
    bool contains_submission(std::string task_id);

    std::expected<task_submission, std::string> get_submission(const std::string& task_id); 
    std::expected<task_result, std::string>     get_result(const std::string& task_id); 

    void change_submission_status(const std::string& id, task_status status); 
private:
    pqxx::connection& conn;
};
