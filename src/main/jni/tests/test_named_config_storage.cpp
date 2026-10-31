#include "../UI/NamedConfigStorage.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
int main(){
    char temp[]="/tmp/ethnir_config_test_XXXXXX";
    char* dir=mkdtemp(temp);
    assert(dir!=nullptr);
    std::string directory=std::string(dir)+"/configs";
    using namespace NamedConfigStorage;
    std::string error;
    assert(!ValidName("../evil"));
    assert(!ValidName(" leading"));
    assert(!ValidName("a/b"));
    assert(ValidName("My profile_01"));
    assert(!Save(directory,"../bad",Json::object(),error));
    error.clear();
    assert(!Save(directory,"autoload",Json::object(),error));
    error.clear();
    Json settings={{"overlay",true},{"size",14.5},{"mode",1},{"palette",{.1,.2,.3}}};
    assert(Save(directory,"My profile_01",settings,error));
    auto files=List(directory);
    assert(files.size()==1 && files[0]=="My profile_01");
    Json loaded;
    assert(Load(directory,"My profile_01",loaded,error));
    assert(loaded==settings);
    assert(SaveAutoLoad(directory,"My profile_01",error));
    assert(ReadAutoLoad(directory)=="My profile_01");
    files=List(directory);
    assert(files.size()==1 && files[0]=="My profile_01");
    settings["overlay"]=false;
    assert(Save(directory,"My profile_01",settings,error));
    assert(Load(directory,"My profile_01",loaded,error) && loaded==settings);
    {std::ofstream bad(ProfilePath(directory,"Broken")); bad<<"not valid json";}
    assert(!Load(directory,"Broken",loaded,error));
    {std::ofstream wrong(ProfilePath(directory,"Old")); wrong<<"{\"version\":2,\"settings\":{}}";}
    assert(!Load(directory,"Old",loaded,error));
    assert(Remove(directory,"My profile_01",error));
    assert(!Load(directory,"My profile_01",loaded,error));
    assert(SaveAutoLoad(directory,"",error));
    assert(ReadAutoLoad(directory).empty());
    std::remove(ProfilePath(directory,"Broken").c_str());
    std::remove(ProfilePath(directory,"Old").c_str());
    std::remove((directory+"/autoload.json").c_str());
    rmdir(directory.c_str());rmdir(dir);
    std::cout<<"Named config storage: all assertions passed\n";
}
