#pragma once

#include <string>

enum language {
    CPP,
    PY
};

enum task_status {
    IN_PROGRESS,
    QUEUED,
    READY
};

struct task {
    language lang;
    task_status status;
    std::string code;
};

struct user {
    std::string username;
    std::string password;

    bool operator==(const user& other) const {
        return username == other.username && password == other.password;
    }
};
