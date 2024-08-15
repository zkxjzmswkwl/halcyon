#include "discord.h"
#include <cpr/cpr.h>
#include <rapidjson/document.h>

//---------------------------------------------------------------------------
std::string post_login(std::string username, std::string password)
{
  std::string body(R"({
    "login": ")" + username + R"(",
    "password": ")" + password + R"(",
    "undelete": false,
    "login_source": null,
    "gift_code_sku_id": null
  })");

  cpr::Response r = cpr::Post(
    cpr::Url{"https://discord.com/api/v9/auth/login"},
    cpr::Header{{"content-type", "application/json"}},
    cpr::Timeout{10000},
    cpr::Body{body}
  );

  rapidjson::Document doc;
  doc.Parse(r.text.c_str());

  if (r.status_code != 200)
    return "";

  if (doc["token"].IsString())
    return doc["token"].GetString();

  return "";
}
/*---------------------------------------------------------------------------
* Allocates `DiscordContext* ctx`
* Passes it to `get_servers(ctx)`
* `get_servers(ctx)` allocates one `Server*` for each server user is in,
* which are added to `ctx->servers`.
*
* For each server user is in, calls `get_channels_for_server(ctx, Server*)`,
* which allocates one `Channel*` for each channel in the given server.
* Which are then pushed to `server->channels`.
---------------------------------------------------------------------------*/
DiscordContext* get_context(std::string token)
{
  DiscordContext* ctx = new DiscordContext(token);
  if (!get_servers(ctx))
    printf("ERR: Could not get servers.\n");

  return ctx;
}
//---------------------------------------------------------------------------
bool get_servers(DiscordContext* ctx)
{
  cpr::Response r = cpr::Get(
    cpr::Url{"https://discord.com/api/v9/users/@me/guilds"},
    cpr::Header{{"Authorization", ctx->token}},
    cpr::Header{{"User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) discord/0.0.63 Chrome/124.0.6367.243 Electron/30.2.0 Safari/537.36"}},
    cpr::Timeout{10000}
  );

  rapidjson::Document doc;
  doc.Parse(r.text.c_str());

  assert(r.status_code == 200);
  assert(doc.IsArray());

  for (int i = 0; i < doc.Size(); ++i)
  {
    const rapidjson::Value& item = doc[i];
    Server* newServer = new Server(item["name"].GetString(), item["id"].GetString());
    ctx->servers[i] = newServer;
    ctx->server_count++;
    if (!get_channels_for_server(ctx, newServer))
      printf("ERR: Could not retrieve channel listing for server %s\n", newServer->name.c_str());
  }

  return true;
}
//---------------------------------------------------------------------------
bool get_channels_for_server(DiscordContext* ctx, Server* server)
{
  cpr::Response r = cpr::Get(
    cpr::Url{"https://discord.com/api/v9/guilds/" + server->id + "/channels"},
    cpr::Header{{"Authorization", ctx->token}},
    cpr::Header{{"User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) discord/0.0.63 Chrome/124.0.6367.243 Electron/30.2.0 Safari/537.36"}},
    cpr::Timeout{10000}
  );

  printf("Getting channels for server %s\n", server->name.c_str());

  rapidjson::Document doc;
  doc.Parse(r.text.c_str());

  if (!doc.IsArray())
    return false;

  for (int i = 0; i < doc.Size(); ++i)
  {
    const rapidjson::Value& item = doc[i];

    server->channels[i] = new Channel(
      static_cast<ChannelType>(item["type"].GetUint()),
      item["name"].GetString(),
      item["id"].GetString()
    );
  }

  return true;
}
//---------------------------------------------------------------------------
bool post_message(DiscordContext* ctx, std::string message, std::string channelId)
{
  std::string body("{\"content\": \"" + message + "\"}");
  cpr::Response r = cpr::Post(
    cpr::Url{"https://discord.com/api/v9/channels/" + channelId + "/messages"},
    cpr::Header{{"content-type", "application/json"}},
    cpr::Header{{"Authorization", ctx->token}},
    cpr::Timeout{10000},
    cpr::Body{body}
  );

  return true;
}
//---------------------------------------------------------------------------
