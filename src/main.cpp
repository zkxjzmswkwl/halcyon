#include "discord.h"
#include "stream.h"
#include "terminal.h"
#include <assert.h>
#include <fstream>
#include <intrin.h>
#include <iostream>
#include <string>
#include <thread>

discord::DiscordContext* g_ctx;

void render()
{
    auto f_server = g_ctx->focused_server;
    auto f_channel = g_ctx->focused_channel;

    term::clear();
    // Inefficient/bad

    // focused_server won't be populated if messaging a user directly.
    if (f_server)
    {
        term::print_label("Server: ");
        term::print_label(f_server->name);
    }
    term::print_label(" Channel: ");
    term::print_label(f_channel->name);
    // -
    printf("\n\n");

    for (auto&& message : g_ctx->focused_channel->messages)
    {
        term::print_green(message->author);
        printf("\n  %s\n", message->body.c_str());
    }
}

void switch_channel_focus(discord::Channel* channel)
{
    g_ctx->focused_channel = channel;
    discord::get_focused_channel_content(g_ctx);
}

/// TODO: Move to input handler of some sort.
/// Hacky poc for now.
void take_input()
{
    std::string input;
    std::getline(std::cin, input);

    if (input == "/list")
    {
        display_servers(g_ctx);
    }
    else if (input.starts_with("/say"))
    {
        auto delim_index = input.find(' ');
        auto msg_body = input.substr(delim_index + 1, input.length());
        post_message(g_ctx, msg_body, g_ctx->focused_channel->id);
        get_focused_channel_content(g_ctx);
        render();
    }
    else if (input == "/fr")
    {
        discord::display_friends(g_ctx);
    }
    else if (input.starts_with("/fr"))
    {
        auto delim_index = input.find(' ');
        auto chosen_index = input.substr(delim_index + 1, input.length());
        auto _friend = g_ctx->friends.at(atoi(chosen_index.c_str()));

        auto private_chat_id = discord::get_chat_id_from_user(g_ctx, _friend->id);
        // This is a memory leak.
        // Too bad!
        // TODO: fix.
        auto channel =
            new discord::Channel(discord::ChannelType::TEXT, "Private chat with " + _friend->username, private_chat_id);
        switch_channel_focus(channel);
    }
    else if (input.starts_with("/srv"))
    {
        auto delim_index = input.find(' ');
        // given "/srv 1", want just "1".
        auto server_choice = input.substr(delim_index + 1, input.length());
        g_ctx->focused_server = g_ctx->servers.at(atoi(server_choice.c_str()));
        display_channels_for_server(g_ctx->focused_server, discord::ChannelType::TEXT);
    }
    else if (input.starts_with("/ch") && g_ctx->focused_server != nullptr)
    {
        auto delim_index = input.find(' ');
        auto channel_choice = input.substr(delim_index + 1, input.length());
        auto focused_channel = g_ctx->focused_server->channels.at(atoi(channel_choice.c_str()));
        switch_channel_focus(focused_channel);
    }
    else if (input == "/cls")
    {
        term::clear();
    }
    else if (input == "/r")
    {
        render();
    }
    else if (input == "/q" || input == "/exit")
    {
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    take_input();
}

std::string login_get_token()
{
    // Read account.bin
    std::ifstream account_bin("C:\\Users\\owcar\\halcyon\\account.bin");
    std::string line;
    std::getline(account_bin, line);
    auto delim_index = line.find(':');
    std::string email = line.substr(0, delim_index);
    std::string password = line.substr(delim_index + 1, line.length());

    // login
    auto token = discord::post_login(email, password);
    assert(token.length() > 0);
    return token;
}

std::string read_token_bin()
{
    std::ifstream account_bin("C:\\Users\\owcar\\halcyon\\token.bin");
    std::string line;
    std::getline(account_bin, line);
    return line;
}

int main(int, char**)
{
    g_ctx = discord::get_context(read_token_bin());
    // it works so it must be fine >.<
    std::thread _t_websocket(discord::stream::open, (__int64*)g_ctx);

    take_input();

    // This matters! /s
    delete g_ctx;
}
