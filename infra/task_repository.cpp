#include <random>

#include "infra/task_repository.hpp"
#include "lib/random.hpp"

std::string task_repository::add(task t) {
    auto uuid = random_uuid();        
    tasks.insert({uuid, t});
    return uuid;
}

task task_repository::get(std::string task_id) {
    auto it = tasks.find(task_id);
    if (it == tasks.end())
        throw new std::invalid_argument("no task with id " + task_id);
    return it->second;
}

void task_repository::remove(std::string task_id) {
    auto it = tasks.find(task_id);
    if (it == tasks.end())
        throw new std::invalid_argument("no task with id " + task_id);
    tasks.erase(it);
}

void task_repository::change_status(std::string id, task_status status) {
    tasks[id].status = status;
}

void task_repository::clear() {
    tasks.clear();
}

bool task_repository::contains(std::string task_id) {
    return tasks.contains(task_id);
}
