#pragma once

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "log.h"

namespace captain {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name)
        ,m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}

    const std::string& getName() const { return m_name;}
    const std::string& getDescription() const { return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    //virtual std::string getTypeName() const = 0;
protected:
    std::string m_name;
    std::string m_description;
};

//FromStr T operator()(const std::string&) 
//ToStr std::string operator()(const T&)
template<class T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    //typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;

    ConfigVar(const std::string& name
            ,const T& default_value
            ,const std::string& description = "")
        :ConfigVarBase(name, description)
        ,m_val(default_value) {
    }

    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
            //return ToStr()(m_val);
        } catch (std::exception& e) {
            CAPTAIN_LOG_ERROR(CAPTAIN_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
            //setValue(FromStr()(val));
        } catch (std::exception& e) {
            CAPTAIN_LOG_ERROR(CAPTAIN_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: string to " << typeid(m_val).name()
                << " - " << val;
        }
        return false;
    }

    const T getValue() const { return m_val;}
    void setValue(const T& v) { m_val = v;}

private:
    T m_val;
};

class Config {
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_value, const std::string& description = ""){
        auto tmp = Lookup<T>(name);
        if(tmp) {
            CAPTAIN_LOG_INFO(CAPTAIN_LOG_ROOT()) << "Lookup name=" << name << " exists";
            return tmp;
        }

        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")!= std::string::npos) {
            CAPTAIN_LOG_ERROR(CAPTAIN_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        auto it = s_datas.find(name);
        if(it == s_datas.end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }
 
    static void LoadFromYaml(const YAML::Node& root);
    static ConfigVarBase::ptr LookupBase(const std::string& name);

    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
private:
    static ConfigVarMap s_datas;
};

}

