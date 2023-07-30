#include "include/config.h"

namespace captain {
    // Config::ConfigVarMap Config::s_datas;

    // ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    //     auto it = s_datas.find(name);  //在 s_datas 中查找键为 name 的元素。
    //     return it == s_datas.end() ? nullptr : it->second;
    // }

    ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);  //在 s_datas 中查找键为 name 的元素。
        return it == GetDatas().end() ? nullptr : it->second;
    }

    static void ListAllMember(const std::string& prefix,
                            const YAML::Node& node,
                            std::list<std::pair<std::string, const YAML::Node> >& output) {
        //检查prefix是否包含除了小写字母、下划线和数字以外的字符。
        if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
                != std::string::npos) {
            CAPTAIN_LOG_ERROR(CAPTAIN_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        //如果是Map类型，说明该节点有子节点，需要继续递归遍历子节点。
        if(node.IsMap()) {
            for(auto it = node.begin();
                    it != node.end(); ++it) {
                //将当前子节点作为新的根节点，继续遍历子节点及其子节点
                ListAllMember(prefix.empty() ? it->first.Scalar()
                        : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node& root) {
        std::list<std::pair<std::string, const YAML::Node> > all_nodes;
        ListAllMember("", root, all_nodes);

        for(auto& i : all_nodes) {
            std::string key = i.first;
            if(key.empty()) {
                continue;
            }
            //将键转换为小写，以确保不区分大小写的键查找 转换后的结果将被存储在key中
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if(var) {
                if(i.second.IsScalar()) {
                    var->fromString(i.second.Scalar());
                } else {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin();
            it != m.end(); ++it) {
        cb(it->second);
    }
}

}
