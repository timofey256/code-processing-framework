#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "helpers/rabbitmq.hpp"
#include "helpers/time.hpp"

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

            // TODO: run your actual program inside Docker here
            std::string result = "ok"; // fake result for now

            // TODO: send HTTP POST to /commit with result + task_id
            // e.g. using libcurl or asio-http

            std::cout << "Committed result for task " << task_id << std::endl;

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

