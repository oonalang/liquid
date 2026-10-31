#include "../src/main/jni/Includes/EthnirAuthReply.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace ethnir_auth;
 assert(!ParseSignedReply("", "K", "ID").accepted);
 assert(ParseSignedReply("<html>Not found</html>","K","ID").error.find("HTML")!=std::string::npos);
 assert(!ParseSignedReply("{invalid", "K", "ID").accepted);
 assert(!ParseSignedReply(R"({"status":true,"data":{"token":"x","rng":123}})","K","ID").accepted);
 assert(ParseSignedReply(R"({"status":true,"data":{"token":"x","rng":123}})","K","ID").error.find("signed session")!=std::string::npos);
 assert(!ParseSignedReply(R"({"status":true,"data":{"payload":"abc","sig":"###"}})","K","ID").accepted);
 assert(!ParseSignedReply(R"({"status":false,"reason":"Key expired"})", "K", "ID").accepted);
 assert(ParseSignedReply(R"({"status":false,"reason":"Key expired"})", "K", "ID").error=="Key expired");
 std::cout<<"PASSED: signed-response parsing checks (8 assertions)\n";
}
