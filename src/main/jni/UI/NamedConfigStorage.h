#pragma once
#include "../Noctivagous/json.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>



namespace NamedConfigStorage {
using Json = nlohmann::json;
inline bool ValidName(const std::string& name) {
    if (name.empty() || name.size() > 40 || name.front()==' ' || name.back()==' ') return false;
    for (unsigned char c : name)
        if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
              (c>='0'&&c<='9')||c==' '||c=='_'||c=='-')) return false;
    return name!="." && name!="..";
}
inline bool EnsureDirectory(const std::string& dir) {
    if (dir.empty()) return false;
    struct stat st{};
    if (stat(dir.c_str(), &st)==0) return S_ISDIR(st.st_mode);
    return mkdir(dir.c_str(),0700)==0 || (errno==EEXIST && stat(dir.c_str(),&st)==0 && S_ISDIR(st.st_mode));
}
inline std::string ProfilePath(const std::string& directory, const std::string& name) {
    return directory+"/"+name+".json";
}
inline bool AtomicWrite(const std::string& path, const std::string& contents, std::string& error) {
    const std::string temporary=path+".tmp";
    {
        std::ofstream file(temporary, std::ios::binary|std::ios::trunc);
        if (!file || !file.write(contents.data(),static_cast<std::streamsize>(contents.size()))) {
            error="Cannot write profile to app storage";
            std::remove(temporary.c_str());
            return false;
        }
        file.flush();
        if (!file) {error="Could not finish writing profile"; std::remove(temporary.c_str());return false;}
    }
    if (std::rename(temporary.c_str(),path.c_str())!=0) {
        error="Cannot replace profile";
        std::remove(temporary.c_str());
        return false;
    }
    return true;
}
inline bool Save(const std::string& directory,const std::string& name,const Json& settings,std::string& error) {
    if (!ValidName(name) || name=="autoload") {error="Use a valid name other than autoload (1-40 characters)";return false;}
    if (!EnsureDirectory(directory)) {error="Config folder is unavailable";return false;}
    if (!settings.is_object()) {error="Settings must be a JSON object";return false;}
    Json document={{"version",1},{"settings",settings}};
    std::string payload=document.dump(2);
    if(payload.size()>128*1024) {error="Profile exceeds size limit";return false;}
    return AtomicWrite(ProfilePath(directory,name),payload,error);
}
inline bool Load(const std::string& directory,const std::string& name,Json& settings,std::string& error) {
    if (!ValidName(name)) {error="Invalid profile name";return false;}
    std::ifstream file(ProfilePath(directory,name),std::ios::binary|std::ios::ate);
    if (!file) {error="Profile not found";return false;}
    const auto size=file.tellg();
    if (size<=0 || size>static_cast<std::streamoff>(128*1024)) {error="Invalid profile size";return false;}
    file.seekg(0);
    try {
        Json document=Json::parse(file);
        if (!document.is_object()||document.value("version",0)!=1||
            !document.contains("settings")||!document["settings"].is_object()) {
            error="Unsupported profile version or format";return false;
        }
        settings=document["settings"];
        return true;
    } catch (const std::exception&) {error="Malformed profile JSON";return false;}
}
inline std::vector<std::string> List(const std::string& directory) {
    std::vector<std::string> names;
    DIR* dir=opendir(directory.c_str());
    if(!dir) return names;
    while (dirent* entry=readdir(dir)) {
        const std::string filename(entry->d_name);
        if(filename.size()<6 || filename.substr(filename.size()-5)!=".json") continue;
        const std::string name=filename.substr(0,filename.size()-5);
        if(ValidName(name) && name!="autoload") names.push_back(name);
    }
    closedir(dir);
    std::sort(names.begin(),names.end());
    return names;
}
inline bool Remove(const std::string& directory,const std::string& name,std::string& error) {
    if(!ValidName(name)) {error="Invalid profile name";return false;}
    if(std::remove(ProfilePath(directory,name).c_str())!=0) {error="Could not delete profile";return false;}
    return true;
}
inline bool SaveAutoLoad(const std::string& directory,const std::string& name,std::string& error) {
    if(!name.empty()&&!ValidName(name)) {error="Invalid autoload profile";return false;}
    if(!EnsureDirectory(directory)) {error="Config folder is unavailable";return false;}
    return AtomicWrite(directory+"/autoload.json",Json({{"version",1},{"name",name}}).dump(2),error);
}
inline std::string ReadAutoLoad(const std::string& directory) {
    std::ifstream file(directory+"/autoload.json",std::ios::binary|std::ios::ate);
    if (!file||file.tellg()<=0||file.tellg()>2048) return {};
    file.seekg(0);
    try {
        const Json data=Json::parse(file);
        if (!data.is_object()||data.value("version",0)!=1) return {};
        const std::string name=data.value("name",std::string());
        return name.empty()||ValidName(name)?name:std::string();
    } catch (const std::exception&) {return {};}
}
} // namespace NamedConfigStorage
