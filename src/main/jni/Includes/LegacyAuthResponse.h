#pragma once

#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <string>

namespace shai_auth {

struct Response {
    bool accepted = false;
    std::string error;
    std::string token;
    std::string expiry;
    std::string modStatus;
};

inline Response ParseLegacyReply(const std::string& body, std::time_t now = std::time(nullptr)) {
    Response result;
    const auto start = body.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        result.error = "The license panel returned an empty response.";
        return result;
    }
    if (body[start] == '<') {
        result.error = "The panel returned HTML, not JSON. Check shaii.x10.mx/connect and hosting protection.";
        return result;
    }
    if (body[start] != '{') {
        result.error = "The license panel returned an unexpected response format.";
        return result;
    }
    try {
        const nlohmann::json parsed = nlohmann::json::parse(body);
        if (!parsed.is_object() || !parsed.contains("status") || !parsed["status"].is_boolean()) {
            result.error = "The panel response is missing a valid status field.";
            return result;
        }
        if (!parsed["status"].get<bool>()) {
            result.error = parsed.contains("reason") && parsed["reason"].is_string()
                ? parsed["reason"].get<std::string>() : "The panel rejected this license key.";
            if (result.error.empty()) result.error = "The panel rejected this license key.";
            if (result.error.size() > 220) result.error.resize(220);
            return result;
        }
        if (!parsed.contains("data") || !parsed["data"].is_object()) {
            result.error = "The panel accepted the request but returned no license data.";
            return result;
        }
        const auto& data = parsed["data"];
        if (!data.contains("token") || !data["token"].is_string() ||
            data["token"].get<std::string>().empty()) {
            result.error = "The license panel response has no token.";
            return result;
        }
        // Require the original panel protocol's freshness field. Do not infer success
        // from a web page, a missing token, or an incomplete JSON response.
        if (!data.contains("rng") ||
            !(data["rng"].is_number_integer() || data["rng"].is_number_unsigned())) {
            result.error = "The panel response is missing a valid session timestamp (rng).";
            return result;
        }
        const std::int64_t issued = data["rng"].get<std::int64_t>();
        const std::int64_t current = static_cast<std::int64_t>(now);
        if (issued < current - 120 || issued > current + 120) {
            result.error = "The panel session timestamp has expired or the device clock is incorrect.";
            return result;
        }
        result.token = data["token"].get<std::string>();
        if (data.contains("EXP") && data["EXP"].is_string())
            result.expiry = data["EXP"].get<std::string>();
        if (data.contains("mod_status") && data["mod_status"].is_string())
            result.modStatus = data["mod_status"].get<std::string>();
        result.accepted = true;
        return result;
    } catch (const nlohmann::json::exception&) {
        result.error = "The license panel returned malformed JSON.";
        return result;
    }
}

} // namespace shai_auth
