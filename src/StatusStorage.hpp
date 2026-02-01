#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>


struct StoredNode
{
    std::string id;
    std::string name;
    std::string url;
    bool online = false;
    std::string last_ping;
};

namespace StatusStorage
{
    // Load nodes (missing file -> empty list, all_online defaults to true)
    std::vector<StoredNode> load();
    // Save nodes
    void save(std::vector<StoredNode> const &nodes);

    // Helpers
    std::optional<StoredNode> getById(std::vector<StoredNode> const &nodes, std::string const &id);
    void upsertNode(std::vector<StoredNode> &nodes, StoredNode const &node);
    void removeById(std::vector<StoredNode> &nodes, std::string const &id);
}
