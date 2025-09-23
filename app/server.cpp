#include <asio.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <chrono>
#include <thread>
#include <expected>
#include <nlohmann/json.hpp>
#include <cstdlib> 
#include <charconv>
#include <optional>

#include "core/types.hpp"

#include "infra/types.hpp"
#include "infra/dispatcher.hpp"
#include "infra/request_parser.hpp"
#include "infra/types.hpp"
#include "infra/task_repository.hpp"

#include "services/auth_service.hpp"

#include "lib/env.hpp"

using asio::ip::tcp;


int main() {
    int port = get_port("PORT").value_or(8081);

    task_repository task_repo;
    auth_service auth_s;
    request_parser rp;
    dispatcher disp(auth_s);

    disp.add_route(request_type::GET, "/", [](const request&, const std::unordered_map<std::string, std::string>&) {
        return response { 200, "text/plain", "Hello world from Code Processor!" };
    }, true);

    disp.add_route(request_type::POST, "/register", [&](const request& req, const std::unordered_map<std::string, std::string>&) {
        try {
            nlohmann::json payload = nlohmann::json::parse(req.body);

            std::string username = payload.at("username").get<std::string>();
            std::string password = payload.at("password").get<std::string>();

            std::cout << "Registering user: " << username << "\n";
            auth_s.register_user(username, password);

            return response{201, "text/plain", "Successfully registered!"};
        } catch (std::exception& e) {
            return response{400, "application/json",
            std::string("{\"error\":\"bad request: ") + e.what() + "\"}"};
        }
    }, true);


    disp.add_route(request_type::POST, "/login", [&](const request& req, const std::unordered_map<std::string, std::string>&) {
        nlohmann::json payload = nlohmann::json::parse(req.body);

        std::string username = payload.at("username").get<std::string>();
        std::string password = payload.at("password").get<std::string>();

        auto token = auth_s.login_user(username, password);
        if (!token) {
            return response { 401, "text/plain", "Unauthorized Error" };
        }
        std::cout << "Logging in user: " << username << "\n";
        return response { 200, "application/json", "{ \"token\": \"" + *token + "\" }"  };
    }, true);

    disp.add_route(request_type::POST, "/task", [&](const request& r, const std::unordered_map<std::string, std::string>&) {
        std::string task_id = task_repo.add(task{language::CPP, task_status::IN_PROGRESS, ""});
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        task_repo.change_status(task_id, task_status::READY);

        return response { 201, "application/json", "{ \"task_id\": \"" + task_id + "\" }"  };
    });

    disp.add_route(request_type::GET, "/status/{task_id}", [&](const request&, const std::unordered_map<std::string, std::string>& params) {
        auto it = params.find("task_id");
        if (it == params.end())
            return response {404, "text/plain", "Invalid request: couldn't read task_id" };
        auto task_id = it->second;
        if (!task_repo.contains(task_id))
            return response {404, "text/plain", "Not found: task wih id " + task_id + " does not exist" };

        return response { 200, "application/json", "{ \"status\": \"ready\" }" };
    });

    disp.add_route(request_type::GET, "/result/{task_id}", [&](const request&, const std::unordered_map<std::string, std::string>& params) {
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
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));

        std::cout << "Server listening on http://localhost:" << port << std::endl;

        for (;;) {
            tcp::socket socket(io);
            acceptor.accept(socket);

            asio::streambuf buffer;
            asio::read_until(socket, buffer, "\r\n\r\n");
            std::istream request_stream(&buffer);
            std::expected<request, std::string> r = rp.parse(request_stream);
            if (!r) {
                std::cerr << "Failed to parse the request: " << r.error() << "\n";
                response e = response {400, "text/plain", "Invalid request" };
                asio::write(socket, asio::buffer(e.to_string()));
                continue;
            }

            std::cout << "Type: "   << r->type << std::endl;
            std::cout << "Auth: "   << r->headers["Authorization"] << std::endl;
            std::cout << "Target: " << r->target << std::endl;
            std::cout << "Body: "   << r->body << std::endl;

            response resp = disp.dispatch(*r);

            std::cout << "Response: " << resp.to_string() << std::endl;

            asio::write(socket, asio::buffer(resp.to_string()));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
