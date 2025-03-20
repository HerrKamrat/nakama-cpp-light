/*
 * Copyright 2019 The Nakama Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include <nlohmann/json.hpp>
#include "DefaultSession.h"
#include "nakama-cpp/NSessionInterface.h"
using Json = nlohmann::json;

#include "RestClient.h"
#include "StrUtil.h"

#include "grpc_status_code_enum.h"
#include "nakama-cpp/NakamaVersion.h"
#include "nakama-cpp/log/NLogger.h"
#include "nakama-cpp/realtime/NWebsocketsFactory.h"
#undef NMODULE_NAME
#define NMODULE_NAME "Nakama::RestClient"

using namespace std;

namespace Nakama {

///< A collection of zero or more friends of the user.
struct NSessionData {
    bool created;
    std::string token;
    std::string refreshToken;
};

template <>
struct DataWriter<NSessionData> {
    static NSessionData from_json_string(const std::string& jsonStr) {
        NSessionData data;
        Json document = Json::parse(jsonStr);

        if (document.contains("created"))
            document["created"].get_to(data.created);
        document["token"].get_to(data.token);
        document["refresh_token"].get_to(data.refreshToken);

        return data;
    }
};

void AddBoolArg(NHttpQueryArgs& args, string&& name, bool value) {
    value ? args.emplace(name, "true") : args.emplace(name, "false");
}

string jsonDocToStr(Json& document) {
    return document.dump();
}

void addVarsToJsonDoc(Json& document, const NStringMap& vars) {
    if (!vars.empty()) {
        Json jsonObj = {};

        for (auto& p : vars) {
            jsonObj[p.first] = p.second;
        }

        document["vars"] = jsonObj;
    }
}

RestClient::RestClient(const NClientParameters& parameters, NHttpTransportPtr httpClient) : _httpClient(std::move(httpClient)) {
    NLOG(NLogLevel::Info, "Created. NakamaSdkVersion: %s", getNakamaSdkVersion());

    _host = parameters.host;
    _ssl = parameters.ssl;
    _platformParams = parameters.platformParams;
    _port = parameters.port;
    std::string baseUrl;

    if (_port == DEFAULT_PORT) {
        _port = parameters.ssl ? 443 : 7350;
        NLOG(NLogLevel::Info, "using default port %d", _port);
    }

    _ssl ? baseUrl.append("https") : baseUrl.append("http");
    baseUrl.append("://").append(parameters.host).append(":").append(std::to_string(_port));

    _httpClient->setBaseUri(baseUrl);

    _basicAuthMetadata = "Basic " + base64Encode(parameters.serverKey + ":");
}

RestClient::~RestClient() {
    disconnect();

    if (_reqContexts.size() > 0) {
        NLOG(NLogLevel::Warn, "Not handled %u request(s) detected.", _reqContexts.size());

        for (RestReqContext* reqContext : _reqContexts) {
            delete reqContext;
        }

        _reqContexts.clear();
    }
}

void RestClient::disconnect() {
    _httpClient->cancelAllRequests();
}

void RestClient::tick() {
    _httpClient->tick();
}

RestReqContext* RestClient::createReqContext(Message* data) {
    RestReqContext* ctx = new RestReqContext();
    ctx->data = data;
    _reqContexts.emplace(ctx);
    return ctx;
}

void RestClient::setBasicAuth(RestReqContext* ctx) {
    ctx->auth.append(_basicAuthMetadata);
}

void RestClient::setSessionAuth(RestReqContext* ctx, NSessionPtr session) {
    ctx->auth.append("Bearer ").append(session->getAuthToken());
}

void RestClient::sendReq(RestReqContext* ctx, NHttpReqMethod method, std::string&& path, std::string&& body, NHttpQueryArgs&& args) {
    NHttpRequest req;

    req.method = method;
    req.path = std::move(path);
    req.body = std::move(body);
    req.queryArgs = std::move(args);

    req.headers.emplace("Accept", "application/json");
    req.headers.emplace("Content-Type", "application/json");
    if (!ctx->auth.empty())
        req.headers.emplace("Authorization", std::move(ctx->auth));

    _httpClient->request(req, [this, ctx](NHttpResponsePtr response) { onResponse(ctx, response); });
}

void RestClient::onResponse(RestReqContext* reqContext, NHttpResponsePtr response) {
    NLOG_INFO("...");
    auto it = _reqContexts.find(reqContext);

    if (it != _reqContexts.end()) {
        if (response->statusCode == 200)  // OK
        {
            if (reqContext->successCallback) {
                bool ok = true;

                if (reqContext->data) {
                    try {
                        Json document = Json::parse(response->body);
                        reqContext->data->from_json_string(response->body);
                    } catch (const Json::parse_error& e) {
                        NLOG_ERROR("Parse JSON failed for message. Error: " + string(e.what()) + ", json: " + response->body);
                        ok = false;
                    } catch (const Json::type_error& e) {
                        NLOG_ERROR("JSON to Message failed. Error: " + string(e.what()) + ", json: " + response->body);
                        ok = false;
                    }
                    if (!ok) {
                        reqError(reqContext, NError("Parse JSON failed. HTTP body: " + response->body + " error: ", ErrorCode::InternalError));
                    }
                }

                if (ok) {
                    reqContext->successCallback();
                }
            }
        } else {
            std::string errMessage;
            ErrorCode code = ErrorCode::Unknown;

            if (response->statusCode == InternalStatusCodes::CONNECTION_ERROR) {
                code = ErrorCode::ConnectionError;
                errMessage.append("message: ").append(response->errorMessage);
            } else if (response->statusCode == InternalStatusCodes::CANCELLED_BY_USER) {
                code = ErrorCode::CancelledByUser;
                errMessage.append("message: ").append(response->errorMessage);
            } else if (response->statusCode == InternalStatusCodes::INTERNAL_TRANSPORT_ERROR) {
                code = ErrorCode::InternalError;
                errMessage.append("message: ").append(response->errorMessage);
            } else if (!response->body.empty() && response->body[0] == '{')  // have to be JSON
            {
                try {
                    try {
                        Json document = Json::parse(response->body);

                        auto& jsonMessage = document["message"];
                        auto& jsonCode = document["code"];

                        if (jsonMessage.is_string()) {
                            errMessage.append("message: ").append(jsonMessage.get<string>());
                        }

                        if (jsonCode.is_number()) {
                            int serverErrCode = jsonCode.get<int>();

                            switch (serverErrCode) {
                                case grpc::StatusCode::UNAVAILABLE:
                                    code = ErrorCode::ConnectionError;
                                    break;
                                case grpc::StatusCode::INTERNAL:
                                    code = ErrorCode::InternalError;
                                    break;
                                case grpc::StatusCode::NOT_FOUND:
                                    code = ErrorCode::NotFound;
                                    break;
                                case grpc::StatusCode::ALREADY_EXISTS:
                                    code = ErrorCode::AlreadyExists;
                                    break;
                                case grpc::StatusCode::INVALID_ARGUMENT:
                                    code = ErrorCode::InvalidArgument;
                                    break;
                                case grpc::StatusCode::UNAUTHENTICATED:
                                    code = ErrorCode::Unauthenticated;
                                    break;
                                case grpc::StatusCode::PERMISSION_DENIED:
                                    code = ErrorCode::PermissionDenied;
                                    break;

                                default:
                                    errMessage.append("\ncode: ").append(std::to_string(serverErrCode));
                                    break;
                            }
                        }

                    } catch (const Json::parse_error& e) {
                        errMessage = "Parse JSON failed: ";
                        errMessage += e.what();
                        code = ErrorCode::InternalError;
                    }
                } catch (exception& e) {
                    NLOG_ERROR("exception: " + string(e.what()));
                }
            }

            if (errMessage.empty()) {
                errMessage.append("message: ").append(response->errorMessage);
                errMessage.append("\nHTTP status: ").append(std::to_string(response->statusCode));
                errMessage.append("\nbody: ").append(response->body);
            }

            reqError(reqContext, NError(std::move(errMessage), code));
        }

        delete reqContext;
        _reqContexts.erase(it);
    } else {
        reqError(nullptr, NError("Not found request context.", ErrorCode::InternalError));
    }
}

void RestClient::reqError(RestReqContext* reqContext, const NError& error) {
    NLOG_ERROR(error);

    if (reqContext && reqContext->errorCallback) {
        reqContext->errorCallback(error);
    } else if (_defaultErrorCallback) {
        _defaultErrorCallback(error);
    } else {
        NLOG_WARN("^ error not handled");
    }
}

void RestClient::authenticateDevice(const std::string& id,
                                    const opt::optional<std::string>& username,
                                    const opt::optional<bool>& create,
                                    const NStringMap& vars,
                                    std::function<void(NSessionPtr)> successCallback,
                                    ErrorCallback errorCallback) {
    try {
        NLOG_INFO("...");

        auto sessionData(make_shared<DataMessage<NSessionData>>());
        RestReqContext* ctx = createReqContext(sessionData.get());
        setBasicAuth(ctx);

        if (successCallback) {
            ctx->successCallback = [sessionData, successCallback]() {
                NSessionPtr session(new DefaultSession(sessionData->data.token, sessionData->data.refreshToken, sessionData->data.created));
                successCallback(session);
            };
        }
        ctx->errorCallback = errorCallback;

        NHttpQueryArgs args;

        if (username)
            args.emplace("username", encodeURIComponent(*username));

        if (create) {
            AddBoolArg(args, "create", *create);
        }

        nlohmann::json document = {};
        document["id"] = id;
        addVarsToJsonDoc(document, vars);

        string body = jsonDocToStr(document);

        sendReq(ctx, NHttpReqMethod::POST, "/v2/account/authenticate/device", std::move(body), std::move(args));
    } catch (exception& e) {
        NLOG_ERROR("exception: " + string(e.what()));
    }
}

void RestClient::authenticateEmail(const std::string& email,
                                   const std::string& password,
                                   const std::string& username,
                                   bool create,
                                   const NStringMap& vars,
                                   std::function<void(NSessionPtr)> successCallback,
                                   ErrorCallback errorCallback) {
#if 0
                                    try {
        NLOG_INFO("...");

        auto sessionData(make_shared<nakama::api::Session>());
        RestReqContext* ctx = createReqContext(sessionData.get());
        setBasicAuth(ctx);

        if (successCallback) {
            ctx->successCallback = [sessionData, successCallback]() {
                NSessionPtr session(new DefaultSession(sessionData->token(), sessionData->refresh_token(), sessionData->created()));
                successCallback(session);
            };
        }
        ctx->errorCallback = errorCallback;

        NHttpQueryArgs args;

        args.emplace("username", encodeURIComponent(username));
        AddBoolArg(args, "create", create);

        rapidjson::Document document;
        document.SetObject();

        document.AddMember("email", email, document.GetAllocator());
        document.AddMember("password", password, document.GetAllocator());
        addVarsToJsonDoc(document, vars);

        string body = jsonDocToStr(document);

        sendReq(ctx, NHttpReqMethod::POST, "/v2/account/authenticate/email", std::move(body), std::move(args));
    } catch (exception& e) {
        NLOG_ERROR("exception: " + string(e.what()));
    }
#endif
}

void RestClient::authenticateFacebook(const std::string& accessToken,
                                      const std::string& username,
                                      bool create,
                                      bool importFriends,
                                      const NStringMap& vars,
                                      std::function<void(NSessionPtr)> successCallback,
                                      ErrorCallback errorCallback) {
}

void RestClient::authenticateGoogle(const std::string& accessToken,
                                    const std::string& username,
                                    bool create,
                                    const NStringMap& vars,
                                    std::function<void(NSessionPtr)> successCallback,
                                    ErrorCallback errorCallback) {
}

void RestClient::authenticateGameCenter(const std::string& playerId,
                                        const std::string& bundleId,
                                        NTimestamp timestampSeconds,
                                        const std::string& salt,
                                        const std::string& signature,
                                        const std::string& publicKeyUrl,
                                        const std::string& username,
                                        bool create,
                                        const NStringMap& vars,
                                        std::function<void(NSessionPtr)> successCallback,
                                        ErrorCallback errorCallback) {
}

void RestClient::authenticateApple(const std::string& token,
                                   const std::string& username,
                                   bool create,
                                   const NStringMap& vars,
                                   std::function<void(NSessionPtr)> successCallback,
                                   ErrorCallback errorCallback) {
}

void RestClient::authenticateCustom(const std::string& id,
                                    const std::string& username,
                                    bool create,
                                    const NStringMap& vars,
                                    std::function<void(NSessionPtr)> successCallback,
                                    ErrorCallback errorCallback) {
}

void RestClient::authenticateSteam(const std::string& token,
                                   const std::string& username,
                                   bool create,
                                   const NStringMap& vars,
                                   std::function<void(NSessionPtr)> successCallback,
                                   ErrorCallback errorCallback) {
}

void RestClient::authenticateRefresh(NSessionPtr session, std::function<void(NSessionPtr)> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkFacebook(NSessionPtr session, const std::string& accessToken, const opt::optional<bool>& importFriends, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkEmail(NSessionPtr session, const std::string& email, const std::string& password, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkDevice(NSessionPtr session, const std::string& id, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkGoogle(NSessionPtr session, const std::string& accessToken, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkGameCenter(NSessionPtr session,
                                const std::string& playerId,
                                const std::string& bundleId,
                                NTimestamp timestampSeconds,
                                const std::string& salt,
                                const std::string& signature,
                                const std::string& publicKeyUrl,
                                std::function<void()> successCallback,
                                ErrorCallback errorCallback) {
}

void RestClient::linkApple(NSessionPtr session, const std::string& token, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkSteam(NSessionPtr session, const std::string& token, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::linkCustom(NSessionPtr session, const std::string& id, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkFacebook(NSessionPtr session, const std::string& accessToken, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkEmail(NSessionPtr session, const std::string& email, const std::string& password, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkGoogle(NSessionPtr session, const std::string& accessToken, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkGameCenter(NSessionPtr session,
                                  const std::string& playerId,
                                  const std::string& bundleId,
                                  NTimestamp timestampSeconds,
                                  const std::string& salt,
                                  const std::string& signature,
                                  const std::string& publicKeyUrl,
                                  std::function<void()> successCallback,
                                  ErrorCallback errorCallback) {
}

void RestClient::unlinkApple(NSessionPtr session, const std::string& token, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkSteam(NSessionPtr session, const std::string& token, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkDevice(NSessionPtr session, const std::string& id, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::unlinkCustom(NSessionPtr session, const std::string& id, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::importFacebookFriends(NSessionPtr session, const std::string& token, const opt::optional<bool>& reset, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::getAccount(NSessionPtr session, std::function<void(const NAccount&)> successCallback, ErrorCallback errorCallback) {
}

void RestClient::updateAccount(NSessionPtr session,
                               const opt::optional<std::string>& username,
                               const opt::optional<std::string>& displayName,
                               const opt::optional<std::string>& avatarUrl,
                               const opt::optional<std::string>& langTag,
                               const opt::optional<std::string>& location,
                               const opt::optional<std::string>& timezone,
                               std::function<void()> successCallback,
                               ErrorCallback errorCallback) {
}

void RestClient::getUsers(NSessionPtr session,
                          const std::vector<std::string>& ids,
                          const std::vector<std::string>& usernames,
                          const std::vector<std::string>& facebookIds,
                          std::function<void(const NUsers&)> successCallback,
                          ErrorCallback errorCallback) {
}

void RestClient::addFriends(NSessionPtr session, const std::vector<std::string>& ids, const std::vector<std::string>& usernames, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::deleteFriends(NSessionPtr session,
                               const std::vector<std::string>& ids,
                               const std::vector<std::string>& usernames,
                               std::function<void()> successCallback,
                               ErrorCallback errorCallback) {
}

void RestClient::blockFriends(NSessionPtr session, const std::vector<std::string>& ids, const std::vector<std::string>& usernames, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::listFriends(NSessionPtr session,
                             const opt::optional<int32_t>& limit,
                             const opt::optional<NFriend::State>& state,
                             const std::string& cursor,
                             std::function<void(NFriendListPtr)> successCallback,
                             ErrorCallback errorCallback) {
}

void RestClient::createGroup(NSessionPtr session,
                             const std::string& name,
                             const std::string& description,
                             const std::string& avatarUrl,
                             const std::string& langTag,
                             bool open,
                             const opt::optional<int32_t>& maxCount,
                             std::function<void(const NGroup&)> successCallback,
                             ErrorCallback errorCallback) {
}

void RestClient::deleteGroup(NSessionPtr session, const std::string& groupId, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::addGroupUsers(NSessionPtr session, const std::string& groupId, const std::vector<std::string>& ids, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::listGroupUsers(NSessionPtr session,
                                const std::string& groupId,
                                const opt::optional<int32_t>& limit,
                                const opt::optional<NUserGroupState>& state,
                                const std::string& cursor,
                                std::function<void(NGroupUserListPtr)> successCallback,
                                ErrorCallback errorCallback) {
}

void RestClient::kickGroupUsers(NSessionPtr session, const std::string& groupId, const std::vector<std::string>& ids, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::joinGroup(NSessionPtr session, const std::string& groupId, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::leaveGroup(NSessionPtr session, const std::string& groupId, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::listGroups(NSessionPtr session, const std::string& name, int32_t limit, const std::string& cursor, std::function<void(NGroupListPtr)> successCallback, ErrorCallback errorCallback) {
}

void RestClient::listUserGroups(NSessionPtr session,
                                const opt::optional<int32_t>& limit,
                                const opt::optional<NUserGroupState>& state,
                                const std::string& cursor,
                                std::function<void(NUserGroupListPtr)> successCallback,
                                ErrorCallback errorCallback) {
}

void RestClient::listUserGroups(NSessionPtr session,
                                const std::string& userId,
                                const opt::optional<int32_t>& limit,
                                const opt::optional<NUserGroupState>& state,
                                const std::string& cursor,
                                std::function<void(NUserGroupListPtr)> successCallback,
                                ErrorCallback errorCallback) {
}

void RestClient::promoteGroupUsers(NSessionPtr session, const std::string& groupId, const std::vector<std::string>& ids, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::demoteGroupUsers(NSessionPtr session, const std::string& groupId, const std::vector<std::string>& ids, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::updateGroup(NSessionPtr session,
                             const std::string& groupId,
                             const opt::optional<std::string>& name,
                             const opt::optional<std::string>& description,
                             const opt::optional<std::string>& avatarUrl,
                             const opt::optional<std::string>& langTag,
                             const opt::optional<bool>& open,
                             std::function<void()> successCallback,
                             ErrorCallback errorCallback) {
}

void RestClient::listLeaderboardRecords(NSessionPtr session,
                                        const std::string& leaderboardId,
                                        const std::vector<std::string>& ownerIds,
                                        const opt::optional<int32_t>& limit,
                                        const opt::optional<std::string>& cursor,
                                        std::function<void(NLeaderboardRecordListPtr)> successCallback,
                                        ErrorCallback errorCallback) {
}

void RestClient::listLeaderboardRecordsAroundOwner(NSessionPtr session,
                                                   const std::string& leaderboardId,
                                                   const std::string& ownerId,
                                                   const opt::optional<int32_t>& limit,
                                                   std::function<void(NLeaderboardRecordListPtr)> successCallback,
                                                   ErrorCallback errorCallback) {
}

void RestClient::writeLeaderboardRecord(NSessionPtr session,
                                        const std::string& leaderboardId,
                                        std::int64_t score,
                                        const opt::optional<std::int64_t>& subscore,
                                        const opt::optional<std::string>& metadata,
                                        std::function<void(NLeaderboardRecord)> successCallback,
                                        ErrorCallback errorCallback) {
}

void RestClient::writeTournamentRecord(NSessionPtr session,
                                       const std::string& tournamentId,
                                       std::int64_t score,
                                       const opt::optional<std::int64_t>& subscore,
                                       const opt::optional<std::string>& metadata,
                                       std::function<void(NLeaderboardRecord)> successCallback,
                                       ErrorCallback errorCallback) {
}

void RestClient::deleteLeaderboardRecord(NSessionPtr session, const std::string& leaderboardId, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::listMatches(NSessionPtr session,
                             const opt::optional<int32_t>& min_size,
                             const opt::optional<int32_t>& max_size,
                             const opt::optional<int32_t>& limit,
                             const opt::optional<std::string>& label,
                             const opt::optional<std::string>& query,
                             const opt::optional<bool>& authoritative,
                             std::function<void(NMatchListPtr)> successCallback,
                             ErrorCallback errorCallback) {
}

void RestClient::listNotifications(NSessionPtr session,
                                   const opt::optional<int32_t>& limit,
                                   const opt::optional<std::string>& cacheableCursor,
                                   std::function<void(NNotificationListPtr)> successCallback,
                                   ErrorCallback errorCallback) {
}

void RestClient::deleteNotifications(NSessionPtr session, const std::vector<std::string>& notificationIds, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::listChannelMessages(NSessionPtr session,
                                     const std::string& channelId,
                                     const opt::optional<int32_t>& limit,
                                     const opt::optional<std::string>& cursor,
                                     const opt::optional<bool>& forward,
                                     std::function<void(NChannelMessageListPtr)> successCallback,
                                     ErrorCallback errorCallback) {
}

void RestClient::listTournaments(NSessionPtr session,
                                 const opt::optional<uint32_t>& categoryStart,
                                 const opt::optional<uint32_t>& categoryEnd,
                                 const opt::optional<uint32_t>& startTime,
                                 const opt::optional<uint32_t>& endTime,
                                 const opt::optional<int32_t>& limit,
                                 const opt::optional<std::string>& cursor,
                                 std::function<void(NTournamentListPtr)> successCallback,
                                 ErrorCallback errorCallback) {
}

void RestClient::listTournamentRecords(NSessionPtr session,
                                       const std::string& tournamentId,
                                       const opt::optional<int32_t>& limit,
                                       const opt::optional<std::string>& cursor,
                                       const std::vector<std::string>& ownerIds,
                                       std::function<void(NTournamentRecordListPtr)> successCallback,
                                       ErrorCallback errorCallback) {
}

void RestClient::listTournamentRecordsAroundOwner(NSessionPtr session,
                                                  const std::string& tournamentId,
                                                  const std::string& ownerId,
                                                  const opt::optional<int32_t>& limit,
                                                  std::function<void(NTournamentRecordListPtr)> successCallback,
                                                  ErrorCallback errorCallback) {
}

void RestClient::joinTournament(NSessionPtr session, const std::string& tournamentId, std::function<void()> successCallback, ErrorCallback errorCallback) {
    try {
        NLOG_INFO("...");

        RestReqContext* ctx = createReqContext(nullptr);
        setSessionAuth(ctx, session);

        ctx->successCallback = successCallback;
        ctx->errorCallback = errorCallback;

        sendReq(ctx, NHttpReqMethod::POST, "/v2/tournament/" + tournamentId + "/join", "");
    } catch (exception& e) {
        NLOG_ERROR("exception: " + string(e.what()));
    }
}

void RestClient::listStorageObjects(NSessionPtr session,
                                    const std::string& collection,
                                    const opt::optional<int32_t>& limit,
                                    const opt::optional<std::string>& cursor,
                                    std::function<void(NStorageObjectListPtr)> successCallback,
                                    ErrorCallback errorCallback) {
}

void RestClient::listUsersStorageObjects(NSessionPtr session,
                                         const std::string& collection,
                                         const std::string& userId,
                                         const opt::optional<int32_t>& limit,
                                         const opt::optional<std::string>& cursor,
                                         std::function<void(NStorageObjectListPtr)> successCallback,
                                         ErrorCallback errorCallback) {
}

void RestClient::writeStorageObjects(NSessionPtr session,
                                     const std::vector<NStorageObjectWrite>& objects,
                                     std::function<void(const NStorageObjectAcks&)> successCallback,
                                     ErrorCallback errorCallback) {
}

void RestClient::readStorageObjects(NSessionPtr session, const std::vector<NReadStorageObjectId>& objectIds, std::function<void(const NStorageObjects&)> successCallback, ErrorCallback errorCallback) {
}

void RestClient::deleteStorageObjects(NSessionPtr session, const std::vector<NDeleteStorageObjectId>& objectIds, std::function<void()> successCallback, ErrorCallback errorCallback) {
}

void RestClient::rpc(NSessionPtr session, const std::string& id, const opt::optional<std::string>& payload, std::function<void(const NRpc&)> successCallback, ErrorCallback errorCallback) {
}

void RestClient::rpc(const std::string& http_key, const std::string& id, const opt::optional<std::string>& payload, std::function<void(const NRpc&)> successCallback, ErrorCallback errorCallback) {
}

void RestClient::sendRpc(RestReqContext* ctx, const std::string& id, const opt::optional<std::string>& payload, NHttpQueryArgs&& args) {
}

}  // namespace Nakama
