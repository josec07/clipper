#include <iostream>
#include <string>
#include "CLI11.hpp"
#include "cli/commands.h"

int main(int argc, char* argv[]) {
    CLI::App app{"CTIC - Chat Twitch Intelligence CLI"};
    app.require_subcommand(1);
    
    std::string url;
    auto add_cmd = app.add_subcommand("add", "Add a creator to monitor");
    add_cmd->add_option("url", url, "Twitch URL or channel name")->required();
    add_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_add(url));
    });
    
    auto list_cmd = app.add_subcommand("list", "List configured creators");
    list_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_list());
    });
    
    std::string status_name;
    auto status_cmd = app.add_subcommand("status", "Check connection status");
    status_cmd->add_option("name", status_name, "Creator name (optional)");
    status_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_status(status_name));
    });
    
    auto run_cmd = app.add_subcommand("run", "Monitor all creators");
    run_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_run());
    });
    
    std::string remove_name;
    auto remove_cmd = app.add_subcommand("remove", "Remove a creator");
    remove_cmd->add_option("name", remove_name, "Creator name")->required();
    remove_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_remove(remove_name));
    });
    
    CLI11_PARSE(app, argc, argv);
    
    return 0;
}
