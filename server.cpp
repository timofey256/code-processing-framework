#include <asio.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

using asio::ip::tcp;

enum request_type {
    GET,
    POST,
    PUT,
    DELETE_,
    HEAD,
    OPTIONS,
    PATCH
};

class request_parser {
    public:
        struct request {
            request_type type;
            std::string target;
            std::unordered_map<std::string, std::string> headers;
            std::string body;
        };

        request parse(std::istream& serialized) {
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

class dispatcher {
    public:
        using handler_t = std::function<response(const request_parser::request&, const std::unordered_map<std::string, std::string>&)>;

        void add_route(request_type method, const std::string& pattern, handler_t handler) {
            routes_.push_back({method, pattern_to_regex(pattern), extract_keys(pattern), std::move(handler)});
        }

        response dispatch(const request_parser::request& req) const {
            for (auto& route : routes_) {
                if (req.type != route.method) continue;

                std::smatch match;
                if (std::regex_match(req.target, match, route.regex)) {
                    std::unordered_map<std::string, std::string> params;
                    for (size_t i = 0; i < route.keys.size(); ++i) {
                        params[route.keys[i]] = match[i + 1];
                    }
                    return route.handler(req, params);
                }
            }
            return {404, "text/plain", "Not Found"};
        }

    private:
        struct route_entry {
            request_type method;
            std::regex regex;
            std::vector<std::string> keys;
            handler_t handler;
        };

        std::vector<route_entry> routes_;

        static std::regex pattern_to_regex(const std::string& pattern) {
            std::string regex_str;
            std::vector<std::string> keys;
            std::stringstream ss(pattern);
            std::string segment;

            size_t i = 0;
            while (i < pattern.size()) {
                if (pattern[i] == '{') {
                    size_t j = pattern.find('}', i);
                    std::string key = pattern.substr(i + 1, j - i - 1);
                    regex_str += "([^/]+)"; 
                    i = j + 1;
                } else {
                    if (std::isalnum(pattern[i]))
                        regex_str += pattern[i];
                    else {
                        if (std::string(".^$|()[]*+?\\").find(pattern[i]) != std::string::npos)
                            regex_str += '\\';
                        regex_str += pattern[i];
                    }
                    i++;
                }
            }
            return std::regex("^" + regex_str + "$");
        }

        static std::vector<std::string> extract_keys(const std::string& pattern) {
            std::vector<std::string> keys;
            size_t i = 0;
            while ((i = pattern.find('{', i)) != std::string::npos) {
                size_t j = pattern.find('}', i);
                keys.push_back(pattern.substr(i + 1, j - i - 1));
                i = j + 1;
            }
            return keys;
        }
};

std::string random_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, 0xffffffff);

    auto gen32 = [&]() { return dis(gen); };

    uint32_t data[4] = { gen32(), gen32(), gen32(), gen32() };

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << data[0] << "-"
        << std::setw(4) << (data[1] >> 16) << "-"
        << std::setw(4) << ((data[1] & 0x0fff) | 0x4000) << "-" 
        << std::setw(4) << ((data[2] & 0x3fff) | 0x8000) << "-" 
        << std::setw(12) << (((uint64_t)(data[2] & 0xffff) << 32) | data[3]);

    return oss.str();
}

int main() {
    std::unordered_map<std::string, int> task_results;
    request_parser rp;
    dispatcher disp;

    disp.add_route(GET, "/", [](const request_parser::request&, const std::unordered_map<std::string, std::string>&) {
        return response { 200, "text/plain", "Hello world from Code Processor!" };
    });

    disp.add_route(POST, "/task", [&](const request_parser::request& r, const std::unordered_map<std::string, std::string>&) {
        auto uuid = random_uuid();        
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        task_results.emplace(uuid, 0);

        return response { 200, "text/plain", "{ uuid: \"" + uuid + "\" }"  };
    });

    disp.add_route(GET, "/status/{task_id}", [&](const request_parser::request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {404, "text/plain", "Invalid request: couldn't read task_id" };

        return response { 200, "text/plain", "{ status: \"ready\" }" };
    });

    disp.add_route(GET, "/result/{task_id}", [&](const request_parser::request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {400, "text/plain", "Bad request: couldn't read task_id" };
        auto task_id = it->second;
        if (!task_results.contains(task_id))
            return response {404, "text/plain", "Not found: task wih id " + task_id + " does not exist" };

        return response { 200, "text/plain", "{ result: \"" + std::to_string(task_results[task_id]) + "\" }" };
    });

    try {
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8081));

        std::cout << "Server listening on http://localhost:8081\n";

        for (;;) {
            tcp::socket socket(io);
            acceptor.accept(socket);

            asio::streambuf buffer;
            asio::read_until(socket, buffer, "\r\n\r\n");
            std::istream request_stream(&buffer);
            request_parser::request r = rp.parse(request_stream);

            std::cout << "Type: " << r.type << std::endl;
            std::cout << "Target: " << r.target << std::endl;
            std::cout << "Body: " << r.body << std::endl;

            response resp = disp.dispatch(r);

            asio::write(socket, asio::buffer(resp.to_string()));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
