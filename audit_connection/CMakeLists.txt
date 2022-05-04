INCLUDE_DIRECTORIES("/usr/local/include")
LINK_DIRECTORIES("/usr/local/lib64")

MYSQL_ADD_PLUGIN(audit_connection
  audit_connection.cc
  MODULE_ONLY
  MODULE_OUTPUT_NAME "audit_connection"
)
