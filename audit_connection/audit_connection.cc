#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <mysqld_error.h>
#include <stdio.h>
#include <sys/types.h>

#include "lex_string.h"
#include "m_ctype.h"
#include "my_compiler.h"
#include "my_inttypes.h"
#include "my_macros.h"
#include "my_sys.h"
#include "mysql/psi/mysql_mutex.h"
#include "thr_mutex.h"



#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"



/*
  Plugin has been installed.
*/
static bool g_plugin_installed = false;

/*
  Record buffer mutex.
*/
static mysql_mutex_t g_record_buffer_mutex;


auto max_size = 1048576 * 50;
auto max_files = 7;
char audit_log_filename[FN_REFLEN];
char *x=fn_format(audit_log_filename, "audit_connection", "", ".log",MY_REPLACE_EXT | MY_UNPACK_FILENAME);
auto logger = spdlog::rotating_logger_mt("audit_connection", audit_log_filename, max_size, max_files);


static int audit_connection_plugin_init(void *arg MY_ATTRIBUTE((unused))) {
  mysql_mutex_init(PSI_NOT_INSTRUMENTED, &g_record_buffer_mutex,
                   MY_MUTEX_INIT_FAST);

  g_plugin_installed = true;

  return (0);
}


static int audit_connection_plugin_deinit(void *arg MY_ATTRIBUTE((unused))) {
  if (g_plugin_installed == true) {


    mysql_mutex_destroy(&g_record_buffer_mutex);

    g_plugin_installed = false;

    spdlog::drop_all();

  }

  return (0);
}


static int audit_connection_notify(MYSQL_THD thd, mysql_event_class_t event_class,
                             const void *event) {
  
  if (event_class == MYSQL_AUDIT_CONNECTION_CLASS) {
    const struct mysql_event_connection *event_connection =
        (const struct mysql_event_connection *)event;

    char s[256];
    const char *user =  event_connection->user.length > 0 ? event_connection->user.str : "<null user>";
    const char *ip =  event_connection->ip.length > 0 ? event_connection->ip.str : "<localhost>";
    unsigned long conn_id =  event_connection->connection_id;


    switch (event_connection->event_subclass) {
      case MYSQL_AUDIT_CONNECTION_CONNECT:
        snprintf(s, 256, "Connect: %s@%s  conn_id:%lu", user, ip, conn_id);
        logger->info(s);
        logger->flush();
        break;
      case MYSQL_AUDIT_CONNECTION_DISCONNECT:
        snprintf(s, 256, "Disconnect: %s@%s  conn_id:%lu", user, ip, conn_id);
        logger->info(s);
        logger->flush();
        break;
      default:
        break;
    }
  }

  return 0;
}

static struct st_mysql_audit audit_connection_descriptor = {
    MYSQL_AUDIT_INTERFACE_VERSION,
    nullptr,                     
    audit_connection_notify,             /* notify function      */
    {(unsigned long)MYSQL_AUDIT_GENERAL_ALL,
     (unsigned long)MYSQL_AUDIT_CONNECTION_ALL
    }};

mysql_declare_plugin(audit_connection){
    MYSQL_AUDIT_PLUGIN,
    &audit_connection_descriptor,
    "AUDIT_CONNECTION",   
    "sunminlei@gmail.com", 
    "USER CONNECTION/DISCONNECTION AUDIT", 
    PLUGIN_LICENSE_GPL,
    audit_connection_plugin_init,
    nullptr,                
    audit_connection_plugin_deinit, 
    0x0001,                
    nullptr,
    nullptr,
    nullptr,
    0,
} mysql_declare_plugin_end;
