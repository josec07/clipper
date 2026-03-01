#pragma once

#include <string>

namespace ctic {
namespace cli {

int cmd_add(const std::string& url_or_channel);
int cmd_list();
int cmd_status(const std::string& name);
int cmd_run();
int cmd_remove(const std::string& name);

}
}
