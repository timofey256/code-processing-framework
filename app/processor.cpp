#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include <asio.hpp>

#include "helpers/rabbitmq.hpp"
#include "helpers/time.hpp"

#include <fstream>
#include <boost/process.hpp>

using asio::ip::tcp;

struct RunResult {
    std::string stdout_str;
    std::string stderr_str;
    int exit_code;
};

RunResult runCommand(const std::string &cmd) {
    boost::process::ipstream out_stream, err_stream;
    boost::process::child c(
        "/bin/sh", "-c", cmd,
        boost::process::std_out > out_stream,
        boost::process::std_err > err_stream
    );

    std::string out_buf, err_buf, line;
    while (out_stream && std::getline(out_stream, line)) {
        out_buf += line + "\n";
    }
    while (err_stream && std::getline(err_stream, line)) {
        err_buf += line + "\n";
    }
    c.wait();
    return { out_buf, err_buf, c.exit_code() };
}

RunResult runTask(const std::string &lang, const std::string &code, const std::string &task_id) {
    std::string codeFile = "/tmp/task_" + task_id;
    std::string cmd;

    if (lang == "cpp") {
        codeFile += ".cpp";
        std::ofstream ofs(codeFile); ofs << code; ofs.close();
        cmd = "docker run --rm -v /tmp:/workspace runner:latest "
              "bash -c \"g++ /workspace/task_" + task_id + ".cpp "
              "-o /workspace/task_" + task_id + ".out && "
              "/workspace/task_" + task_id + ".out\"";
    } else if (lang == "python3") {
        codeFile += ".py";
        std::ofstream ofs(codeFile); ofs << code; ofs.close();
        cmd = "docker run --rm -v /tmp:/workspace runner:latest "
              "python3 /workspace/task_" + task_id + ".py";
    } else {
        return { "", "Unsupported language: " + lang, 1 };
    }

    return runCommand(cmd);
}

void post_commit(const nlohmann::json& report) {
    try {
        asio::io_context io;
        tcp::resolver resolver(io);
        tcp::resolver::results_type endpoints = resolver.resolve("server", "8081");
        // "server" = service name in docker-compose (your server container)

        tcp::socket socket(io);
        asio::connect(socket, endpoints);

        std::string body = report.dump();
        std::string request =
            "POST /commit HTTP/1.1\r\n"
            "Host: server\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

        asio::write(socket, asio::buffer(request));

        // Read until connection is closed
        asio::streambuf response;
        asio::error_code ec;
        asio::read(socket, response, ec);

        std::istream resp_stream(&response);
        std::string line;
        while (std::getline(resp_stream, line)) {
            std::cout << "[commit response] " << line << "\n";
        }
    } catch (std::exception& e) {
        std::cerr << "Failed to POST commit: " << e.what() << "\n";
    }
}


int main() {
    const char* hostname = "rabbitmq";   // docker-compose service name
    int port = 5672;
    const char* queue = "tasks";

    amqp_connection_state_t conn = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);
    if (!socket) die("creating TCP socket");

    int status = amqp_socket_open(socket, hostname, port);
    if (status) die("opening TCP socket");

    die_on_amqp_error(
        amqp_login(conn, "myvhost", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "myuser", "mypass"),
        "Logging in");

    amqp_channel_open(conn, 1);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

    // declare queue (idempotent, so it's safe if already declared)
    amqp_queue_declare_ok_t* r = amqp_queue_declare(
        conn, 1, amqp_cstring_bytes(queue), 0, 0, 0, 1, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");

    // start consuming
    amqp_basic_consume(conn, 1, amqp_cstring_bytes(queue),
                       amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    std::cout << "Waiting for tasks on queue: " << queue << std::endl;

    while (true) {
        amqp_maybe_release_buffers(conn);
        amqp_envelope_t envelope;
        amqp_rpc_reply_t res = amqp_consume_message(conn, &envelope, NULL, 0);

        if (res.reply_type != AMQP_RESPONSE_NORMAL) {
            std::cerr << "Error consuming message\n";
            break;
        }

        std::string body((char*)envelope.message.body.bytes,
                         envelope.message.body.len);

        try {
            auto j = nlohmann::json::parse(body);
            std::string task_id = j.value("task_id", "unknown");

            std::cout << "Received task: " << j.dump() << std::endl;

            std::string lang = j.value("language", "unknown");
            std::string code = j.value("code", "");

            std::cout << "Starting task " << task_id << std::endl;
            RunResult result = runTask(lang, code, task_id);
            std::cout << "Ended task " << task_id << std::endl;

            nlohmann::json report;
            report["task_id"] = task_id;
            report["stdout"] = result.stdout_str;
            report["stderr"] = result.stderr_str;
            report["exit_code"] = result.exit_code;

            // TODO: send HTTP POST to /commit
            post_commit(report);

            std::cout << "Committed result for task " << task_id
                      << "\n--- stdout ---\n" << result.stdout_str
                      << "\n--- stderr ---\n" << result.stderr_str
                      << "\n(exit " << result.exit_code << ")\n";

        } catch (const std::exception& e) {
            std::cerr << "Invalid JSON: " << e.what() << std::endl;
        }

        amqp_destroy_envelope(&envelope);
    }

    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);

    return 0;
}

