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

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "core/types.hpp"

#include "infra/types.hpp"
#include "infra/dispatcher.hpp"
#include "infra/request_parser.hpp"
#include "infra/types.hpp"
#include "infra/task_repository.hpp"

#include "services/auth_service.hpp"

#include "lib/env.hpp"
#include "helpers/rabbitmq.hpp"

using asio::ip::tcp;

int main() {
    int port = get_port("PORT").value_or(8081);

    // RabbitMQ connection
    amqp_connection_state_t conn = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);

    std::string rabbit_vhost = get_env("RABBITMQ_VHOST").value_or("/");
    std::string rabbit_host  = get_env("RABBITMQ_HOST").value_or("localhost");

    int rabbit_port = get_port("RABBITMQ_PORT").value_or(5672);

    if (!socket) {
        throw std::runtime_error("Failed to create RabbitMQ TCP socket");
    }
    if (amqp_socket_open(socket, rabbit_host.c_str(), rabbit_port)) {
        throw std::runtime_error("Failed to open RabbitMQ TCP socket");
    }

    // login (using env user/pass or default)
    std::string rabbit_user = get_env("RABBITMQ_USER").value_or("guest");
    std::string rabbit_pass = get_env("RABBITMQ_PASS").value_or("guest");

    die_on_amqp_error(amqp_login(conn, rabbit_vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                 rabbit_user.c_str(), rabbit_pass.c_str()),
                      "Logging in");
    
    amqp_channel_open(conn, 1);
    amqp_get_rpc_reply(conn);

    // declare a queue "tasks"
    amqp_queue_declare(conn, 1, amqp_cstring_bytes("tasks"),
                       0, 0, 0, 1, amqp_empty_table);
    amqp_get_rpc_reply(conn);


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

    disp.add_route(request_type::POST, "/task", [&](const request& req, const std::unordered_map<std::string, std::string>&) {
        nlohmann::json payload = nlohmann::json::parse(req.body);

        std::string language = payload.at("language").get<std::string>();
        std::string code = payload.at("code").get<std::string>();

        std::string task_id = task_repo.add(task{language::CPP, task_status::IN_PROGRESS, code});

        nlohmann::json msg_json = {
            {"task_id", task_id},
            {"language", language},
            {"code", code}
        };
        std::string msg = msg_json.dump();

        amqp_basic_publish(conn,
            1,                                   // channel
            amqp_cstring_bytes(""),              // exchange (default)
            amqp_cstring_bytes("tasks"),         // routing key = queue
            0, 0,                                // mandatory, immediate
            NULL,                                // properties
            amqp_cstring_bytes(msg.c_str()));

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

    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
}
