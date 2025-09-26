#pragma once

#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <expected>

#include "core/types.hpp"

class task_repository {
public:
    std::string add_submission(task_submission t); 
    std::string save_result(task_result t); 

    bool contains_result(std::string task_id);
    bool contains_submission(std::string task_id);

    std::expected<task_submission, std::string> get_submission(const std::string& task_id); 
    std::expected<task_result, std::string>     get_result(const std::string& task_id); 

    void change_submission_status(const std::string& id, task_status status); 
    void clear();
private:
    std::unordered_map<std::string, task_submission> task_submissions;
    std::unordered_map<std::string, task_result> task_results;
};
