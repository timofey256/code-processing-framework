#pragma once

#include <string>
#include <chrono>

enum class language {
    CPP,
    PY
};

enum class task_status {
    IN_PROGRESS,
    QUEUED,
    READY
};

struct task_submission {
    language lang;
    task_status status;
    std::string code;
    std::chrono::system_clock::time_point created_at;
};

struct task_result {
    std::string submission_id;

    std::string stdout_result;
    std::string stderr_result;
    std::string exit_code;

    std::chrono::system_clock::time_point completed_at;
};

struct user {
    std::string username;
    std::string password;

    bool operator==(const user& other) const {
        return username == other.username && password == other.password;
    }
};
