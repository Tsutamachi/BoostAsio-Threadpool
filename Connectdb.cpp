#include "Connectdb.h"
using namespace std;
Connectdb::Connectdb()
{
    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
    }
    if (conn) {
        printf("Connected to the database successfully!\n");
    } else {
        fprintf(stderr, "Connection failed\n");
    }
}
void Connectdb::dbclosed()
{
    mysql_close(conn);
    printf("Connection closed successfully.\n");
}
