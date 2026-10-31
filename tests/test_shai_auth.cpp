#include "Includes/LegacyAuthResponse.h"
#include <cassert>
#include <iostream>

int main() {
    using shai_auth::ParseLegacyReply;
    constexpr std::time_t now = 1900000000;
    const std::string ok = R"({"status":true,"data":{"token":"some-server-token","rng":1900000000,"EXP":"2027-01-01","mod_status":"active"}})";
    assert(ParseLegacyReply(ok, now).accepted);
    assert(ParseLegacyReply(ok, now).token == "some-server-token");
    assert(ParseLegacyReply("  <!DOCTYPE html><html></html>", now).error.find("HTML") != std::string::npos);
    assert(!ParseLegacyReply("",now).accepted);
    assert(!ParseLegacyReply("not-json",now).accepted);
    assert(!ParseLegacyReply("{broken",now).accepted);
    assert(!ParseLegacyReply(R"({"status":true,"data":{"rng":1900000000}})",now).accepted);
    assert(!ParseLegacyReply(R"({"status":true,"data":{"token":"X"}})",now).accepted);
    assert(!ParseLegacyReply(R"({"status":true,"data":{"token":"X","rng":1899999800}})",now).accepted);
    assert(!ParseLegacyReply(R"({"status":true,"data":{"token":"X","rng":1900000200}})",now).accepted);
    assert(!ParseLegacyReply(R"({"status":false,"reason":"Invalid license"})",now).accepted);
    assert(ParseLegacyReply(R"({"status":false,"reason":"Invalid license"})",now).error == "Invalid license");
    assert(!ParseLegacyReply(R"({"status":true,"data":{"token":"X","rng":1900000000.5}})",now).accepted);
    assert(!ParseLegacyReply(R"({"status":null})",now).accepted);
    std::cout << "PASS: 13 auth-response checks\n";
}
