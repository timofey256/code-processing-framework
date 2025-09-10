#pragma once

#include <string>

enum request_type {
    GET,
    POST,
    PUT,
    DELETE_,
    HEAD,
    OPTIONS,
    PATCH
};

struct request {
    request_type type;
    std::string target;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct response {
    int status;
    std::string content_type;
    std::string body;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status << " "
            << (status == 200 ? "OK" : "Error") << "\r\n"
            << "Content-Type: " << content_type << "\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "\r\n"
            << body;
        return oss.str();
    }
};

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
