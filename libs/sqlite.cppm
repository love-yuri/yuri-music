//
// Created by love-yuri on 2026/11/25.
//
module;
#include "sqlite3/sqlite3.h"

export module sqlite;

import std;

namespace sqlite {

export using Value =
  std::variant<std::nullptr_t, std::int64_t, double, std::string_view>; // SQL 参数

/** 项目数据库连接。SQL 操作可从多个线程调用，SQLite 错误通过异常报告。 */
export class Database {
public:
  /**
   * 打开或创建项目数据库，首次初始化时执行 SQL 脚本。
   * @param file 数据库文件路径
   * @param init_script 设置 PRAGMA user_version 的初始化 SQL 文本
   */
  explicit Database(const std::filesystem::path &file, std::string_view init_script);

  /**
   * 执行一条无结果的 SQL 语句。
   * @param sql SQL 语句
   * @param values 按占位符顺序绑定的参数
   */
  void execute(std::string_view sql, std::initializer_list<Value> values = {}) const;

  /**
   * 执行可包含多条 SQL 语句的脚本。
   * @param sql SQL 脚本文本
   */
  void execute_script(std::string_view sql) const;

  /**
   * 查询第一行第一列的文本值。
   * @param sql 查询语句
   * @param values 按占位符顺序绑定的参数
   * @return 文本值，无结果或 SQL NULL 时为空
   */
  [[nodiscard]] std::optional<std::string>
  query_text(std::string_view sql, std::initializer_list<Value> values = {}) const;

  /**
   * 查询第一行第一列的整数值。
   * @param sql 查询语句
   * @param values 按占位符顺序绑定的参数
   * @return 整数值，无结果或 SQL NULL 时为空
   */
  [[nodiscard]] std::optional<std::int64_t>
  query_integer(std::string_view sql, std::initializer_list<Value> values = {}) const;

private:
  std::unique_ptr<sqlite3, decltype(&sqlite3_close_v2)> connection_; // SQLite 连接
  mutable std::mutex mutex_;                                         // 串行化连接操作
};

/**
 * 首次访问时打开当前工作目录下的 yuri-music.db，初始化时执行内置 SQL。
 * @return 项目共用的数据库连接
 */
export Database &database();

/**
 * 检查 SQLite 操作结果。
 * @param result SQLite 返回码
 * @param connection SQLite 连接
 */
void check_result(const int result, sqlite3 *const connection) {
  if (result != SQLITE_OK) {
    const auto *const message = connection ? sqlite3_errmsg(connection) : sqlite3_errstr(result);
    throw std::runtime_error(std::format("SQLite ({}): {}", result, message));
  }
}

/**
 * 打开 SQLite 连接并设置锁等待时间。
 * @param file 数据库文件路径
 * @return 管理 SQLite 连接的指针
 */
auto open_connection(const std::filesystem::path &file) {
  const auto name = file.u8string();
  sqlite3 *handle = nullptr;
  constexpr int kOpenFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
  const int result =
    sqlite3_open_v2(reinterpret_cast<const char *>(name.c_str()), &handle, kOpenFlags, nullptr);
  std::unique_ptr<sqlite3, decltype(&sqlite3_close_v2)> connection(handle, sqlite3_close_v2);
  check_result(result, handle);
  check_result(sqlite3_busy_timeout(handle, 5000), handle);
  return connection;
}

/**
 * 将一个参数绑定到语句。
 * @param statement SQLite 语句
 * @param index 参数序号，从 1 开始
 * @param value 参数值
 * @return SQLite 返回码
 */
int bind_value(sqlite3_stmt *const statement, const int index, const Value &value) {
  if (const auto *const text = std::get_if<std::string_view>(&value)) {
    const auto *const data = text->empty() ? "" : text->data();
    return sqlite3_bind_text64(statement, index, data, text->size(), SQLITE_TRANSIENT, SQLITE_UTF8);
  }
  if (const auto *const integer = std::get_if<std::int64_t>(&value)) {
    return sqlite3_bind_int64(statement, index, *integer);
  }
  if (const auto *const number = std::get_if<double>(&value)) {
    return sqlite3_bind_double(statement, index, *number);
  }
  return sqlite3_bind_null(statement, index);
}

/**
 * 编译一条 SQL 语句并绑定参数。
 * @param connection SQLite 连接
 * @param sql SQL 语句
 * @param values 按占位符顺序绑定的参数
 * @return 管理 SQLite 语句的指针
 */
auto prepare(
  sqlite3 *const connection,
  const std::string_view sql,
  const std::initializer_list<Value> values
) {
  const std::string text(sql);
  const char *tail = nullptr;
  sqlite3_stmt *handle = nullptr;
  const int result = sqlite3_prepare_v2(connection, text.c_str(), -1, &handle, &tail);
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(handle, sqlite3_finalize);
  check_result(result, connection);

  constexpr std::string_view kSqlWhitespace = " \t\r\n\v\f"; // SQL 语句末尾允许的空白
  if (
    !statement || sql.contains('\0')
    || std::string_view(tail).find_first_not_of(kSqlWhitespace) != std::string_view::npos
  ) {
    throw std::invalid_argument("需要一条 SQL 语句");
  }
  if (values.size() != static_cast<std::size_t>(sqlite3_bind_parameter_count(handle))) {
    throw std::invalid_argument("SQL 参数与占位符数量不匹配");
  }
  int index = 1;
  for (const auto &value : values) {
    check_result(bind_value(handle, index++, value), connection);
  }
  return statement;
}

/**
 * 执行语句直到取得一行结果或完成。
 * @param statement SQLite 语句
 * @return 是否取得一行结果
 */
bool step_row(sqlite3_stmt *const statement) {
  const int result = sqlite3_step(statement);
  if (result == SQLITE_ROW) {
    return true;
  }
  if (result != SQLITE_DONE) {
    check_result(result, sqlite3_db_handle(statement));
  }
  return false;
}

Database::Database(const std::filesystem::path &file, const std::string_view init_script) :
  connection_(open_connection(file)) {
  if (query_integer("PRAGMA user_version") != 0) {
    return;
  }

  // 取得写锁后再次检查版本，避免两个进程同时初始化。
  execute_script("BEGIN IMMEDIATE");
  try {
    if (query_integer("PRAGMA user_version") == 0) {
      execute_script(init_script);
      if (query_integer("PRAGMA user_version") == 0) {
        throw std::runtime_error("SQLite 初始化脚本未设置数据库版本");
      }
    }
    execute_script("COMMIT");
  } catch (...) {
    sqlite3_exec(connection_.get(), "ROLLBACK", nullptr, nullptr, nullptr);
    throw;
  }
}

void Database::execute(
  const std::string_view sql,
  const std::initializer_list<Value> values
) const {
  const std::lock_guard lock(mutex_);
  const auto statement = prepare(connection_.get(), sql, values);
  if (sqlite3_column_count(statement.get()) != 0) {
    throw std::invalid_argument("SQL 语句返回了结果行");
  }
  static_cast<void>(step_row(statement.get()));
}

void Database::execute_script(const std::string_view sql) const {
  if (sql.contains('\0')) {
    throw std::invalid_argument("SQL 脚本包含空字符");
  }
  const std::lock_guard lock(mutex_);
  const std::string text(sql);
  check_result(
    sqlite3_exec(connection_.get(), text.c_str(), nullptr, nullptr, nullptr), connection_.get()
  );
}

std::optional<std::string>
Database::query_text(const std::string_view sql, const std::initializer_list<Value> values) const {
  const std::lock_guard lock(mutex_);
  const auto statement = prepare(connection_.get(), sql, values);
  if (!step_row(statement.get()) || sqlite3_column_type(statement.get(), 0) == SQLITE_NULL) {
    return std::nullopt;
  }
  const auto *const data = sqlite3_column_text(statement.get(), 0);
  if (!data) {
    check_result(sqlite3_errcode(connection_.get()), connection_.get());
    return std::nullopt;
  }
  return std::string(
    reinterpret_cast<const char *>(data), sqlite3_column_bytes(statement.get(), 0)
  );
}

std::optional<std::int64_t> Database::query_integer(
  const std::string_view sql,
  const std::initializer_list<Value> values
) const {
  const std::lock_guard lock(mutex_);
  const auto statement = prepare(connection_.get(), sql, values);
  if (!step_row(statement.get()) || sqlite3_column_type(statement.get(), 0) == SQLITE_NULL) {
    return std::nullopt;
  }
  return sqlite3_column_int64(statement.get(), 0);
}

Database &database() {
  constexpr std::string_view kInitScript = R"sql(
CREATE TABLE IF NOT EXISTS song_cache (
  mid TEXT NOT NULL,
  format TEXT NOT NULL,
  file_name TEXT NOT NULL,
  PRIMARY KEY (mid, format)
) WITHOUT ROWID;

PRAGMA user_version = 1;
)sql";
  static Database instance("yuri-music.db", kInitScript); // 项目数据库服务
  return instance;
}

} // namespace sqlite
