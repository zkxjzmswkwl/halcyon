#include <string>
#include <assert.h>
#include <iostream>
#include <thread>
#include "discord.h"

DiscordContext* g_ctx;

void display_servers()
{
  for (int i = 0; i < g_ctx->server_count; i++)
  {
    auto server = g_ctx->servers.at(i);
    if (server == nullptr)
      break;

    printf("[%d]\t%s\n", i, server->name.c_str());
  }
}

void take_input()
{
  std::string input;
  std::cin >> input;

  if (input == "/list")
    display_servers();
  else if (input == "/q" || input == "/exit")
      return;

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  take_input();
}

int main(int, char**)
{
  auto token = post_login("email", "password!");
  assert(token.length() > 0);
  g_ctx = get_context(token);
  /*post_message(g_ctx, "Dongs", "1273467180326977663");*/

  take_input();

  // This matters! /s
  delete g_ctx;
}
