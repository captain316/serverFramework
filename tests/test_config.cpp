#include "../captain/include/config.h"
#include "../captain/include/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

captain::ConfigVar<int>::ptr g_int_value_config =
    captain::Config::Lookup("system.port", (int)8080, "system port");

captain::ConfigVar<float>::ptr g_float_value_config =
    captain::Config::Lookup("system.value", (float)10.2f, "system value");

//打印YAML节点（YAML::Node）的内容。根据节点的类型，它将打印节点的值和类型，并递归地打印子节点。
void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << std::string(level * 4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/lk/gitRepo/serverFramework/bin/conf/log.yml");
    //CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << root;
    print_yaml(root, 0);
    //CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << root.Scalar();

    // CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << root["test"].IsDefined();
    // CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << root["logs"].IsDefined();
    // CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << root;
}

void test_config() {
    CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << "before: " << g_float_value_config->toString();

    YAML::Node root = YAML::LoadFile("/home/lk/gitRepo/serverFramework/bin/conf/log.yml");
    captain::Config::LoadFromYaml(root);

    CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << "after: " << g_float_value_config->toString();
}

int main(int argc, char** argv) {
    test_yaml();
    test_config();

    return 0;
}
