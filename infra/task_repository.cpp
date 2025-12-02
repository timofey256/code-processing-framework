#include <random>

#include "infra/task_repository.hpp"
#include "lib/random.hpp"

std::string task_repository::add_submission(task_submission t) {
    auto uuid = random_uuid();
    pqxx::work txn(conn);
    txn.exec_params("INSERT INTO task_submissions (id, language, status, code, created_at) "
                    "VALUES ($1, $2, $3, $4, to_timestamp($5))",
                    uuid,
                    (t.lang == language::CPP ? "cpp" : "python3"),
                    (t.status == task_status::IN_PROGRESS ? "in_progress" : "queued"),
                    t.code,
                    std::chrono::system_clock::to_time_t(t.created_at));
    txn.commit();
    return uuid;
}

bool task_repository::contains_submission(std::string task_id) {
    pqxx::work txn(conn);
    auto r = txn.exec_params("SELECT 1 FROM task_submissions WHERE id=$1", task_id);
    return !r.empty();
}

std::string task_repository::save_result(task_result t) {
    pqxx::work txn(conn);
    txn.exec_params(
        "INSERT INTO task_results (submission_id, stdout, stderr, exit_code, completed_at) "
        "VALUES ($1, $2, $3, $4, to_timestamp($5))",
        t.submission_id,
        t.stdout_result,
        t.stderr_result,
        t.exit_code,
        std::chrono::system_clock::to_time_t(t.completed_at)
    );

    txn.commit();
    return t.submission_id;
}

bool task_repository::contains_result(std::string task_id) {
    pqxx::work txn(conn);
    auto r = txn.exec_params("SELECT 1 FROM task_results WHERE submission_id=$1", task_id);
    return !r.empty();
}

std::expected<task_submission, std::string> task_repository::get_submission(const std::string& task_id) {
    pqxx::work txn(conn);
    auto r = txn.exec_params("SELECT language, status, code, created_at FROM task_submissions WHERE id=$1", task_id);
    if (r.empty()) return std::unexpected("no task submission with id " + task_id);

    language lang = (r[0][0].as<std::string>() == "cpp" ? language::CPP : language::PY);
    task_status st = (r[0][1].as<std::string>() == "ready" ? task_status::READY : task_status::IN_PROGRESS);

    return task_submission{lang, st, r[0][2].as<std::string>(), std::chrono::system_clock::now()};
}

std::expected<task_result, std::string> task_repository::get_result(const std::string& task_id) {
    pqxx::work txn(conn);
    auto r = txn.exec_params("SELECT stdout, stderr, exit_code, completed_at FROM task_results WHERE submission_id=$1", task_id);
    if (r.empty()) return std::unexpected("no task result with id " + task_id);

    return task_result{task_id,
                       r[0][0].as<std::string>(),
                       r[0][1].as<std::string>(),
                       r[0][2].as<std::string>(),
                       std::chrono::system_clock::now()};
}

void task_repository::change_submission_status(const std::string& id, task_status status) {
    pqxx::work txn(conn);
    txn.exec_params("UPDATE task_submissions SET status=$2 WHERE id=$1",
                    id, (status == task_status::READY ? "ready" : "in_progress"));
    txn.commit();
}
