#include <random>

#include "infra/task_repository.hpp"
#include "lib/random.hpp"

std::string task_repository::add(task t) {
    auto uuid = random_uuid();        
    tasks.insert({uuid, t});
    return uuid;
}

std::expected<task, std::string> task_repository::get(const std::string& task_id) {
    auto it = tasks.find(task_id);
    if (it == tasks.end())
        return std::unexpected("no task with id " + task_id);
    return it->second;
}

std::expected<std::string, std::string> task_repository::remove(const std::string& task_id) {
    auto it = tasks.find(task_id);
    if (it == tasks.end())
        return std::unexpected("no task with id " + task_id);
    tasks.erase(it);
    return task_id;
}

void task_repository::change_status(const std::string& id, task_status status) {
    tasks[id].status = status;
}

void task_repository::clear() {
    tasks.clear();
}

bool task_repository::contains(const std::string& task_id) {
    return tasks.contains(task_id);
}
