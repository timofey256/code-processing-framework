#include <random>

#include "infra/task_repository.hpp"
#include "lib/random.hpp"

std::string task_repository::add_submission(task_submission t) {
    auto uuid = random_uuid();        
    task_submissions.insert({uuid, t});
    return uuid;
}

bool task_repository::contains_submission(std::string task_id) {
    return task_submissions.contains(task_id);
}

std::string task_repository::save_result(task_result t) {
    auto uuid = random_uuid();        
    task_results.insert({uuid, t});
    return uuid;
}

bool task_repository::contains_result(std::string task_id) {
    return task_results.contains(task_id);
}

std::expected<task_submission, std::string> task_repository::get_submission(const std::string& task_id) {
    auto it = task_submissions.find(task_id);
    if (it == task_submissions.end())
        return std::unexpected("no task submission with id " + task_id);
    return it->second;
}

std::expected<task_result, std::string> task_repository::get_result(const std::string& task_id) {
    auto it = task_results.find(task_id);
if (it == task_results.end())
        return std::unexpected("no task result with id " + task_id);
return it->second;
}

void task_repository::change_submission_status(const std::string& id, task_status status) {
    task_submissions[id].status = status;
}

void task_repository::clear() {
    task_submissions.clear();
    task_results.clear();
}
