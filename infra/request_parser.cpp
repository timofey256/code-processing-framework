#include "infra/request_parser.hpp"

std::expected<request, std::string> request_parser::parse(std::istream& serialized) {
    request r;

    // parse request line
    std::string request_line;
    std::getline(serialized, request_line);
    if (!request_line.empty() && request_line.back() == '\r')
        request_line.pop_back();

    std::istringstream line_stream(request_line);
    std::string method, path, version;
    line_stream >> method >> path >> version;

    if (method == "GET")          r.type = request_type::GET;
    else if (method == "POST")    r.type = request_type::POST;
    else if (method == "PUT")     r.type = request_type::PUT;
    else if (method == "DELETE")  r.type = request_type::DELETE_;
    else if (method == "HEAD")    r.type = request_type::HEAD;
    else if (method == "OPTIONS") r.type = request_type::OPTIONS;
    else if (method == "PATCH")   r.type = request_type::PATCH;
    else return std::unexpected("Unsupported HTTP method: " + method);

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
        int length = 0; std::stoi(it->second);
        auto [ptr, ec] = std::from_chars(it->second.data(), it->second.data() + it->second.size(), length);

        if (ec != std::errc() || length < 0) {
            return std::unexpected("Invalid Content-Length header");
        }

        r.body.resize(length);
        serialized.read(&r.body[0], length);
    }

    return r;
}
