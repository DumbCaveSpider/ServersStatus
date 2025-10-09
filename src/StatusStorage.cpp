#include "StatusStorage.hpp"
#include <matjson.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace geode::prelude;

namespace
{
    static std::string storagePath()
    {
        // store under the mod's save directory
        return (Mod::get()->getSaveDir() / "status.json").string();
    }

    static bool computeAllOnline(std::vector<StoredNode> const &nodes)
    {
        if (nodes.empty())
            return true;
        for (auto const &n : nodes)
            if (!n.online)
                return false;
        return true;
    }
}

std::vector<StoredNode> StatusStorage::load()
{
    std::vector<StoredNode> out;
    std::ifstream in(storagePath(), std::ios::in | std::ios::binary);
    if (!in.is_open())
        return out;
    std::ostringstream ss;
    ss << in.rdbuf();
    auto str = ss.str();
    auto json = matjson::parse(str).unwrapOr(matjson::Value());
    auto nodesVal = json["nodes"];
    if (nodesVal.isArray())
    {
        for (auto const &v : nodesVal)
        {
            StoredNode n;
            n.id = v["id"].asString().unwrapOr("");
            n.name = v["name"].asString().unwrapOr("");
            n.url = v["url"].asString().unwrapOr("");
            n.online = v["online"].asBool().unwrapOr(false);
            if (!n.id.empty())
                out.push_back(std::move(n));
        }
    }
    return out;
}

void StatusStorage::save(std::vector<StoredNode> const &nodes)
{
    std::vector<matjson::Value> arr;
    arr.reserve(nodes.size());
    for (auto const &n : nodes)
    {
        matjson::Value o;
        o.set("id", n.id);
        o.set("name", n.name);
        o.set("url", n.url);
        o.set("online", n.online);
        arr.emplace_back(o);
    }
    matjson::Value root;
    root.set("nodes", arr);
    root.set("all_online", computeAllOnline(nodes));
    auto dump = root.dump();
    std::ofstream out(storagePath(), std::ios::out | std::ios::binary);
    if (out.is_open())
    {
        out << dump;
        out.close();
    }
}

std::optional<StoredNode> StatusStorage::getById(std::vector<StoredNode> const &nodes, std::string const &id)
{
    for (auto const &n : nodes)
        if (n.id == id)
            return n;
    return std::nullopt;
}

void StatusStorage::upsertNode(std::vector<StoredNode> &nodes, StoredNode const &node)
{
    for (auto &n : nodes)
        if (n.id == node.id)
        {
            n = node;
            return;
        }
    nodes.push_back(node);
}

void StatusStorage::removeById(std::vector<StoredNode> &nodes, std::string const &id)
{
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&](auto const &n)
                               { return n.id == id; }),
                nodes.end());
}
