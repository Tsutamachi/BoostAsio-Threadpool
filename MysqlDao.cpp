#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlDao::MysqlDao()
{
    auto & cfg = ConfigMgr::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host,port, user, pwd,schema, 5));
}

MysqlDao::~MysqlDao(){
    pool_->Close();
}
int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& server) {
    auto conn_ptr = pool_->getConnection();
    if (!conn_ptr || !conn_ptr->_con) {
        std::cerr << "Failed to get connection from pool\n";
        return -1;
    }

    MYSQL* conn = conn_ptr->_con.get();
    MYSQL_STMT* stmt = nullptr;
    MYSQL_BIND binds[4] = {};
    MYSQL_RES* result = nullptr;
    int result_code = 0;

    try {
        // 开启事务
        if (mysql_query(conn, "START TRANSACTION") != 0) {
            std::cerr << "START TRANSACTION failed: " << mysql_error(conn) << std::endl;
            throw std::runtime_error("Failed to start transaction");
        }

        // 1. 检查邮箱是否已存在
        stmt = mysql_stmt_init(conn);
        if (!stmt) {
            std::cerr << "mysql_stmt_init failed: " << mysql_error(conn) << std::endl;
            throw std::runtime_error("Failed to initialize statement");
        }

        const char* check_query = "SELECT COUNT(*) FROM users WHERE email = ?";
        if (mysql_stmt_prepare(stmt, check_query, strlen(check_query)) != 0) {
            std::cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to prepare check statement");
        }

        // 绑定邮箱参数
        binds[0].buffer_type = MYSQL_TYPE_STRING;
        binds[0].buffer = const_cast<char*>(email.data());
        binds[0].buffer_length = email.size();

        if (mysql_stmt_bind_param(stmt, binds) != 0) {
            std::cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to bind check parameters");
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to execute check statement");
        }

        // 获取查询结果
        result = mysql_stmt_result_metadata(stmt);
        if (!result) {
            std::cerr << "mysql_stmt_result_metadata failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to get result metadata");
        }

        // 绑定结果集
        MYSQL_BIND result_bind[1] = {};
        unsigned long count = 0;
        result_bind[0].buffer_type = MYSQL_TYPE_LONG;
        result_bind[0].buffer = &count;
        result_bind[0].is_null = 0;
        result_bind[0].length = 0;

        if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
            std::cerr << "mysql_stmt_bind_result failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to bind result");
        }

        // 获取结果
        if (mysql_stmt_fetch(stmt) != 0) {
            std::cerr << "mysql_stmt_fetch failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to fetch result");
        }

        mysql_free_result(result);
        mysql_stmt_close(stmt);
        stmt = nullptr;

        // 如果邮箱已存在，返回错误
        if (count > 0) {
            mysql_query(conn, "ROLLBACK");
            return 1; // 邮箱已存在
        }

        // 2. 插入新用户
        stmt = mysql_stmt_init(conn);
        if (!stmt) {
            std::cerr << "mysql_stmt_init failed: " << mysql_error(conn) << std::endl;
            throw std::runtime_error("Failed to initialize insert statement");
        }

        const char* insert_query = "INSERT INTO users (username, password, email, server) VALUES (?, ?, ?, ?)";
        if (mysql_stmt_prepare(stmt, insert_query, strlen(insert_query)) != 0) {
            std::cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to prepare insert statement");
        }

        // 绑定插入参数
        // 注意：顺序应与SQL中的字段顺序一致
        binds[0].buffer_type = MYSQL_TYPE_STRING;
        binds[0].buffer = const_cast<char*>(name.data());
        binds[0].buffer_length = name.size();

        binds[1].buffer_type = MYSQL_TYPE_STRING;
        binds[1].buffer = const_cast<char*>(pwd.data());
        binds[1].buffer_length = pwd.size();

        binds[2].buffer_type = MYSQL_TYPE_STRING;
        binds[2].buffer = const_cast<char*>(email.data());
        binds[2].buffer_length = email.size();

        binds[3].buffer_type = MYSQL_TYPE_STRING;
        binds[3].buffer = const_cast<char*>(server.data());
        binds[3].buffer_length = server.size();

        if (mysql_stmt_bind_param(stmt, binds) != 0) {
            std::cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << std::endl;
            throw std::runtime_error("Failed to bind insert parameters");
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << std::endl;
            // 检查是否为唯一约束冲突
            if (strstr(mysql_stmt_error(stmt), "Duplicate entry") != nullptr) {
                mysql_query(conn, "ROLLBACK");
                return 1; // 邮箱已存在（可能是并发插入导致）
            }
            throw std::runtime_error("Failed to execute insert statement");
        }

        mysql_stmt_close(stmt);
        stmt = nullptr;

        // 提交事务
        if (mysql_query(conn, "COMMIT") != 0) {
            std::cerr << "COMMIT failed: " << mysql_error(conn) << std::endl;
            throw std::runtime_error("Failed to commit transaction");
        }

        return 0; // 注册成功
    } catch (const std::exception& e) {
        // 回滚事务（如果有异常）
        if (conn) {
            mysql_query(conn, "ROLLBACK");
        }

        std::cerr << "RegUser failed: " << e.what() << std::endl;
        return -1; // 其他错误
    }
        // 释放资源
        if (stmt) {
            mysql_stmt_close(stmt);
        }
        if (result) {
            mysql_free_result(result);
        }
        if (conn_ptr) {
            pool_->returnConnection(std::move(conn_ptr));
        }
}



// int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd,
// 	const std::string& icon)
// {
// 	auto con = pool_->getConnection();
// 	if (con == nullptr) {
// 		return false;
// 	}

// 	Defer defer([this, &con] {
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try {
// 		con->_con->setAutoCommit(false);
// 		std::unique_ptr<sql::PreparedStatement> pstmt_email(con->_con->prepareStatement("SELECT 1 FROM user WHERE email = ?"));

// 		pstmt_email->setString(1, email);

// 		std::unique_ptr<sql::ResultSet> res_email(pstmt_email->executeQuery());

// 		auto email_exist = res_email->next();
// 		if (email_exist) {
// 			con->_con->rollback();
// 			std::cout << "email " << email << " exist";
// 			return 0;
// 		}

// 		std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("SELECT 1 FROM user WHERE name = ?"));

// 		pstmt_name->setString(1, name);

// 		std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());

// 		auto name_exist = res_name->next();
// 		if (name_exist) {
// 			con->_con->rollback();
// 			std::cout << "name " << name << " exist";
// 			return 0;
// 		}


// 		std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("UPDATE user_id SET id = id + 1"));

// 		// Ö´ÐÐ¸üÐÂ
// 		pstmt_upid->executeUpdate();

// 		// »ñÈ¡¸üÐÂºóµÄ id Öµ
// 		std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("SELECT id FROM user_id"));
// 		std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
// 		int newId = 0;
// 		// ´¦Àí½á¹û¼¯
// 		if (res_uid->next()) {
// 			newId = res_uid->getInt("id");
// 		}
// 		else {
// 			std::cout << "select id from user_id failed" << std::endl;
// 			con->_con->rollback();
// 			return -1;
// 		}

// 		// ²åÈëuserÐÅÏ¢
// 		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement("INSERT INTO user (uid, name, email, pwd, nick, icon) "
// 			"VALUES (?, ?, ?, ?,?,?)"));
// 		pstmt_insert->setInt(1,newId);
// 		pstmt_insert->setString(2, name);
// 		pstmt_insert->setString(3, email);
// 		pstmt_insert->setString(4, pwd);
// 		pstmt_insert->setString(5, name);
// 		pstmt_insert->setString(6, icon);
// 		//Ö´ÐÐ²åÈë
// 		pstmt_insert->executeUpdate();
// 		// Ìá½»ÊÂÎñ
// 		con->_con->commit();
// 		std::cout << "newuser insert into user success" << std::endl;
// 		return newId;
// 	}
// 	catch (sql::SQLException& e) {
// 		// Èç¹û·¢Éú´íÎó£¬»Ø¹öÊÂÎñ
// 		if (con) {
// 			con->_con->rollback();
// 		}
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return -1;
// 	}
// }
// // ????????????
// bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
//     auto conn_ptr = pool_->getConnection();
//     if (!conn_ptr || !conn_ptr->_con) {
//         std::cerr << "Failed to get connection from pool\n";
//         return false;
//     }

//     MYSQL* conn = conn_ptr->_con.get();
//     MYSQL_STMT* stmt = nullptr;
//     MYSQL_BIND binds[1] = {};
//     MYSQL_RES* result = nullptr;
//     MYSQL_ROW row;

//     try {
//         stmt = mysql_stmt_init(conn);
//         if (!stmt) {
//             std::cerr << "mysql_stmt_init failed: " << mysql_error(conn) << std::endl;
//             throw std::runtime_error("Failed to initialize statement");
//         }

//         const char* query = "SELECT email FROM users WHERE userName = ?";
//         if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
//             std::cerr << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt) << std::endl;
//             throw std::runtime_error("Failed to prepare statement");
//         }

//         binds[0].buffer_type = MYSQL_TYPE_STRING;
//         binds[0].buffer = const_cast<char*>(name.data()); // NOLINT(modernize-cast)
//         binds[0].buffer_length = name.size();

//         if (mysql_stmt_bind_param(stmt, binds) != 0) {
//             std::cerr << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt) << std::endl;
//             throw std::runtime_error("Failed to bind parameters");
//         }

//         if (mysql_stmt_execute(stmt) != 0) {
//             std::cerr << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt) << std::endl;
//             throw std::runtime_error("Failed to execute statement");
//         }

//         result = mysql_stmt_result_metadata(stmt);
//         if (!result) {
//             std::cerr << "mysql_stmt_result_metadata failed: " << mysql_stmt_error(stmt) << std::endl;
//             throw std::runtime_error("Failed to get result metadata");
//         }

//         if (mysql_stmt_store_result(stmt) != 0) {
//             std::cerr << "mysql_stmt_store_result failed: " << mysql_stmt_error(stmt) << std::endl;
//             throw std::runtime_error("Failed to store result");
//         }

//         while ((row = mysql_fetch_row(result))) {
//             std::string stored_email = row[0] ? row[0] : "";
//             std::cout << "Check Email: " << stored_email << std::endl;
//             if (email != stored_email) {
//                 mysql_free_result(result);
//                 mysql_stmt_close(stmt);
//                 pool_->returnConnection(std::move(conn_ptr));
//                 return false;
//             }
//             mysql_free_result(result);
//             mysql_stmt_close(stmt);
//             pool_->returnConnection(std::move(conn_ptr));
//             return true;
//         }

//         mysql_free_result(result);
//         mysql_stmt_close(stmt);
//         return false;
//     } catch (const std::exception& e) {
//         std::cerr << "CheckEmail failed: " << e.what() << std::endl;
//         if (result) {
//             mysql_free_result(result);
//         }
//         if (stmt) {
//             mysql_stmt_close(stmt);
//         }
//         pool_->returnConnection(std::move(conn_ptr));
//         return false;
//     }
// }


// bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
//     auto con = pool_->getConnection();
//     try {
//         if (con == nullptr) {
//             return false;
//         }

//         // ×¼±¸²éÑ¯Óï¾ä
//         std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

//         // °ó¶¨²ÎÊý
//         pstmt->setString(2, name);
//         pstmt->setString(1, newpwd);

//         // Ö´ÐÐ¸üÐÂ
//         int updateCount = pstmt->executeUpdate();

//         std::cout << "Updated rows: " << updateCount << std::endl;
//         pool_->returnConnection(std::move(con));
//         return true;
//     }
//     catch (sql::SQLException& e) {
//         pool_->returnConnection(std::move(con));
//         std::cerr << "SQLException: " << e.what();
//         std::cerr << " (MySQL error code: " << e.getErrorCode();
//         std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
//         return false;
//     }
// }

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
    auto conn_ptr = pool_->getConnection();
    if (!conn_ptr || !conn_ptr->_con) {
        std::cerr << "Failed to get connection from pool\n";
        return false;
    }

    MYSQL* conn = conn_ptr->_con.get();
    MYSQL_STMT* stmt = nullptr;//预处理语句句柄
    MYSQL_BIND binds[1] = {};//绑定参数数组
    MYSQL_BIND result_binds[4] = {};//结果集绑定数组
    MYSQL_RES* meta = nullptr;//结果集元数据（所有的结果在一起，需要进一步处理）
    bool success = false;

    try {
        stmt = mysql_stmt_init(conn);//1、创建预处理语句句柄
        if (!stmt) throw std::runtime_error("Failed to initialize statement: " + std::string(mysql_error(conn)));

        const char* query = "SELECT id, userName, email, password FROM users WHERE email = ?";//？为占位符，通过绑定数组进行传递
        if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0)//2、预编译这一条查询语句，只需要编译一次即可多次使用
            throw std::runtime_error("Failed to prepare statement: " + std::string(mysql_stmt_error(stmt)));


        binds[0].buffer_type = MYSQL_TYPE_STRING;
        binds[0].buffer = const_cast<char*>(email.data());
        binds[0].buffer_length = email.size();
        if (mysql_stmt_bind_param(stmt, binds) != 0)// 3、绑定查询参数--email的“ ？”
            throw std::runtime_error("Failed to bind parameters: " + std::string(mysql_stmt_error(stmt)));

        if (mysql_stmt_execute(stmt) != 0)// 4、执行查询
            throw std::runtime_error("Failed to execute statement: " + std::string(mysql_stmt_error(stmt)));

        meta = mysql_stmt_result_metadata(stmt);//获取查询结果的元数据信息
        if (!meta)
            throw std::runtime_error("Failed to get result metadata: " + std::string(mysql_error(conn)));

        // 为结果分配足够空间
        userInfo.name.resize(256);
        userInfo.email.resize(256);
        userInfo.pwd.resize(256);

        unsigned long name_len = 0, email_len = 0, pwd_len = 0;
        my_bool is_null[4] = {0};

        // 绑定结果
        result_binds[0].buffer_type = MYSQL_TYPE_LONG;
        result_binds[0].buffer = &userInfo.uid;
        result_binds[0].is_unsigned = 1;
        result_binds[0].is_null = &is_null[0];

        result_binds[1].buffer_type = MYSQL_TYPE_STRING;
        result_binds[1].buffer = const_cast<char*>(userInfo.name.data());
        result_binds[1].buffer_length = userInfo.name.size();
        result_binds[1].length = &name_len;
        result_binds[1].is_null = &is_null[1];

        result_binds[2].buffer_type = MYSQL_TYPE_STRING;
        result_binds[2].buffer = const_cast<char*>(userInfo.email.data());
        result_binds[2].buffer_length = userInfo.email.size();
        result_binds[2].length = &email_len;
        result_binds[2].is_null = &is_null[2];

        result_binds[3].buffer_type = MYSQL_TYPE_STRING;
        result_binds[3].buffer = const_cast<char*>(userInfo.pwd.data());
        result_binds[3].buffer_length = userInfo.pwd.size();
        result_binds[3].length = &pwd_len;
        result_binds[3].is_null = &is_null[3];

        if (mysql_stmt_bind_result(stmt, result_binds) != 0)// 5、获取结果集
            throw std::runtime_error("Failed to bind result: " + std::string(mysql_stmt_error(stmt)));

        // 6、获取结果
        if (mysql_stmt_fetch(stmt) == MYSQL_NO_DATA) {
            std::cerr << "User not found by email: " << email << std::endl;
            return false;
        }

        // 调整字符串长度为实际读取的长度
        userInfo.name.resize(name_len);
        userInfo.email.resize(email_len);
        userInfo.pwd.resize(pwd_len);

        // 调试输出
        std::cout << "\n--------------" << userInfo.pwd << "-------------" << std::endl;
        std::cout << "Database pwd length: " << userInfo.pwd.length() << std::endl;
        std::cout << "Input pwd length: " << pwd.length() << std::endl;

        // 正确比较字符串内容
        if (userInfo.pwd != pwd) {
            std::cerr << "Password mismatch for email: " << email << std::endl;
            std::cerr << "DB pwd: '" << userInfo.pwd << "'" << std::endl;
            std::cerr << "Input pwd: '" << pwd << "'" << std::endl;
            return false;
        }

        success = true;

    } catch (const std::exception& e) {
        std::cerr << "CheckPwd failed: " << e.what() << std::endl;
        success = false;
    }

    if (stmt) mysql_stmt_close(stmt);//清理资源
    if (meta) mysql_free_result(meta);
    pool_->returnConnection(std::move(conn_ptr));

    return success;
}



// bool MysqlDao::TestProcedure(const std::string& email, int& uid, string& name) {
//     auto con = pool_->getConnection();
//     try {
//         if (con == nullptr) {
//             return false;
//         }

//         Defer defer([this, &con]() {
//             pool_->returnConnection(std::move(con));
//             });
//         unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL test_procedure(?,@userId,@userName)"));
//         stmt->setString(1, email);

//         stmt->execute();
//         unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
//         unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @userId AS uid"));
//         if (!(res->next())) {
//             return false;
//         }

//         uid = res->getInt("uid");
//         cout << "uid: " << uid << endl;

//         stmtResult.reset(con->_con->createStatement());
//         res.reset(stmtResult->executeQuery("SELECT @userName AS name"));
//         if (!(res->next())) {
//             return false;
//         }

//         name = res->getString("name");
//         cout << "name: " << name << endl;
//         return true;

//     }
//     catch (sql::SQLException& e) {
//         std::cerr << "SQLException: " << e.what();
//         std::cerr << " (MySQL error code: " << e.getErrorCode();
//         std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
//         return false;
//     }
// }
