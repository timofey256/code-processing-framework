CREATE TABLE IF NOT EXISTS users (
    username TEXT PRIMARY KEY,
    password TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS task_submissions (
    id UUID PRIMARY KEY,
    language TEXT NOT NULL,
    status TEXT NOT NULL,
    code TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL
);

CREATE TABLE IF NOT EXISTS task_results (
    submission_id UUID PRIMARY KEY REFERENCES task_submissions(id) ON DELETE CASCADE,
    stdout TEXT,
    stderr TEXT,
    exit_code TEXT,
    completed_at TIMESTAMPTZ NOT NULL
);
