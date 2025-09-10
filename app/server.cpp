#include <asio.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <chrono>
#include <thread>

#include "core/types.hpp"
#include "core/dispatcher.hpp"
#include "core/request_parser.hpp"
#include "infra/task_repository.hpp"

using asio::ip::tcp;

int main() {
    task_repository task_repo;
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
        std::string uuid = task_repo.add(task{CPP, IN_PROGRESS, ""});
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        task_repo.change_status(uuid, READY);

        return response { 201, "application/json", "{ \"task_id\": \"" + uuid + "\" }"  };
    });

    disp.add_route(GET, "/status/{task_id}", [&](const request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {404, "text/plain", "Invalid request: couldn't read task_id" };
        auto task_id = it->second;
        if (!task_repo.contains(task_id))
            return response {404, "text/plain", "Not found: task wih id " + task_id + " does not exist" };

        return response { 200, "application/json", "{ \"status\": \"ready\" }" };
    });

    disp.add_route(GET, "/result/{task_id}", [&](const request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {400, "text/plain", "Bad request: couldn't read task_id" };
        auto task_id = it->second;
        if (!task_repo.contains(task_id))
            return response {404, "text/plain", "Not found: task wih id " + task_id + " does not exist" };

        return response { 200, "application/json", "{ \"result\" : \"\" }" };
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
