#include "core/request_parser.hpp"

request request_parser::parse(std::istream& serialized) {
    request r;

    // parse request line
    std::string request_line;
    std::getline(serialized, request_line);
    if (!request_line.empty() && request_line.back() == '\r')
        request_line.pop_back();

    std::istringstream line_stream(request_line);
    std::string method, path, version;
    line_stream >> method >> path >> version;

    if (method == "GET") r.type = GET;
    else if (method == "POST") r.type = POST;
    else if (method == "PUT") r.type = PUT;
    else if (method == "DELETE") r.type = DELETE_;
    else if (method == "HEAD") r.type = HEAD;
    else if (method == "OPTIONS") r.type = OPTIONS;
    else if (method == "PATCH") r.type = PATCH;
    else throw std::runtime_error("Unsupported HTTP method: " + method);

    r.target = path;

    // parse headers
    std::string header;
    while (std::getline(serialized, header) && header != "\r") {
        if (!header.empty() && header.back() == '\r')
            header.pop_back();
        if (header.empty()) break;

        auto colon = header.find(':');
        if (colon != std::string::npos) {
            std::string key = header.substr(0, colon);
            std::string value = header.substr(colon + 1);
            while (!value.empty() && value.front() == ' ')
                value.erase(value.begin());
            r.headers[key] = value;
        }
    }

    // parse body
    auto it = r.headers.find("Content-Length");
    if (it != r.headers.end()) {
        int length = std::stoi(it->second);
        r.body.resize(length);
        serialized.read(&r.body[0], length);
    }

    return r;
}
