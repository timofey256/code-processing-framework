#include <asio.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#include "core/types.hpp"
#include "core/dispatcher.hpp"
#include "core/request_parser.hpp"

using asio::ip::tcp;

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

    disp.add_route(GET, "/", [](const request&, const std::unordered_map<std::string, std::string>&) {
        return response { 200, "text/plain", "Hello world from Code Processor!" };
    });

    disp.add_route(POST, "/register", [](const request&, const std::unordered_map<std::string, std::string>&) {
        return response { 201, "text/plain", "Successfully registered!" };
    });

    disp.add_route(POST, "/login", [](const request&, const std::unordered_map<std::string, std::string>&) {
        return response { 200, "application/json", "{ \"token\": \"0\" }"  };
    });

    disp.add_route(POST, "/task", [&](const request& r, const std::unordered_map<std::string, std::string>&) {
        auto uuid = random_uuid();        
        task_results.emplace(uuid, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        task_results.emplace(uuid, 1);

        return response { 201, "application/json", "{ \"task_id\": \"" + uuid + "\" }"  };
    });

    disp.add_route(GET, "/status/{task_id}", [&](const request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {404, "text/plain", "Invalid request: couldn't read task_id" };
        auto task_id = it->second;
        if (!task_results.contains(task_id))
            return response {404, "text/plain", "Not found: task wih id " + task_id + " does not exist" };

        return response { 200, "application/json", "{ \"status\": \"ready\" }" };
    });

    disp.add_route(GET, "/result/{task_id}", [&](const request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {400, "text/plain", "Bad request: couldn't read task_id" };
        auto task_id = it->second;
        if (!task_results.contains(task_id))
            return response {404, "text/plain", "Not found: task wih id " + task_id + " does not exist" };

        return response { 200, "application/json", "{ \"result\": \"" + std::to_string(task_results[task_id]) + "\" }" };
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
            request r = rp.parse(request_stream);

            std::cout << "Type: " << r.type << std::endl;
            std::cout << "Target: " << r.target << std::endl;
            std::cout << "Body: " << r.body << std::endl;

            response resp = disp.dispatch(r);

            std::cout << "Response: " << resp.to_string() << std::endl;

            asio::write(socket, asio::buffer(resp.to_string()));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
