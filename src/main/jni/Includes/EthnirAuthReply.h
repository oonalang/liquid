#pragma once
#include "json.hpp"
#include "ImGui3SignedSession.h"
#include <ctime>
#include <string>

namespace ethnir_auth {
struct Response {
    bool accepted = false;
    std::string error;
    std::string payload;
    std::time_t expires = 0;
};

inline Response ParseSignedReply(const std::string& body,
                                 const std::string& key,
                                 const std::string& deviceUuid,
                                 std::time_t now = std::time(nullptr)) {
    Response response;
    const size_t first = body.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        response.error = "The license server returned an empty response.";
        return response;
    }
    if (body[first] == '<') {
        response.error = "The /connect route returned HTML, not JSON. Check panel hosting protection.";
        return response;
    }
    if (body[first] != '{') {
        response.error = "The license server returned an unexpected response format.";
        return response;
    }
    try {
        const auto root = nlohmann::json::parse(body);
        if (!root.is_object() || !root.contains("status") || !root["status"].is_boolean()) {
            response.error = "The license response is missing a valid status field.";
            return response;
        }
        if (!root["status"].get<bool>()) {
            response.error = root.contains("reason") && root["reason"].is_string()
                ? root["reason"].get<std::string>() : "The license server rejected this key.";
            if (response.error.empty()) response.error = "The license server rejected this key.";
            if (response.error.size() > 220) response.error.resize(220);
            return response;
        }
        if (!root.contains("data") || !root["data"].is_object()) {
            response.error = "The license response is missing session data.";
            return response;
        }
        const auto& data = root["data"];
        if ((!data.contains("payload") || !data["payload"].is_string()) ||
            (!data.contains("sig") || !data["sig"].is_string())) {
            response.error = "The panel did not return an IMGUI3 signed session (payload and sig).";
            return response;
        }
        const std::string payload = data["payload"].get<std::string>();
        const std::string signature = data["sig"].get<std::string>();
        const auto verdict = imgui3_license::Check(payload, signature, key, deviceUuid, now);
        if (!verdict.ok) {
            response.error = verdict.reason;
            return response;
        }
        response.accepted = true;
        response.payload = payload;
        response.expires = verdict.expires;
        return response;
    } catch (const nlohmann::json::exception&) {
        response.error = "The license server returned malformed JSON.";
        return response;
    }
}
} // namespace ethnir_auth
