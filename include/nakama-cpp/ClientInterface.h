/*
* Copyright 2018 The Nakama Authors
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

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "nonstd/optional.hpp"
#include "nakama-cpp/NSessionInterface.h"
#include "nakama-cpp/NError.h"
#include "nakama-cpp/data/NAccount.h"
#include "nakama-cpp/data/NGroup.h"
#include "nakama-cpp/data/NGroupList.h"
#include "nakama-cpp/data/NGroupUserList.h"
#include "nakama-cpp/data/NUsers.h"
#include "nakama-cpp/data/NUserGroupList.h"
#include "nakama-cpp/data/NFriends.h"

namespace Nakama {

    namespace opt = nonstd;

    typedef std::function<void(const NError&)> ErrorCallback;

    /**
     * A client interface to interact with Nakama server.
     */
    class ClientInterface
    {
    public:
        virtual ~ClientInterface() {}

        /**
         * Disconnects the client. This function kills all outgoing exchanges immediately without waiting.
         */
        virtual void disconnect() = 0;

        /**
         * Pumps requests queue in your thread.
         * Call it periodically, each 50 ms is ok.
         */
        virtual void tick() = 0;

        /**
         * Authenticate a user with a device id.
         *
         * @param id A device identifier usually obtained from a platform API.
         * @param username A username used to create the user. Defaults to empty string.
         * @param create True if the user should be created when authenticated. Defaults to false.
         */
        virtual void authenticateDevice(
            const std::string& id,
            const opt::optional<std::string>& username = opt::nullopt,
            const opt::optional<bool>& create = opt::nullopt,
            std::function<void (NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * Authenticate a user with an email and password.
        *
        * @param email The email address of the user.
        * @param password The password for the user.
        * @param username A username used to create the user.
        * @param create True if the user should be created when authenticated.
        */
        virtual void authenticateEmail(
            const std::string& email,
            const std::string& password,
            const std::string& username = std::string(),
            bool create = false,
            std::function<void(NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * Authenticate a user with a Facebook auth token.
        *
        * @param accessToken An OAuth access token from the Facebook SDK.
        * @param username A username used to create the user.
        * @param create True if the user should be created when authenticated.
        * @param importFriends True if the Facebook friends should be imported.
        */
        virtual void authenticateFacebook(
            const std::string& accessToken,
            const std::string& username = std::string(),
            bool create = false,
            bool importFriends = false,
            std::function<void(NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * Authenticate a user with a Google auth token.
        *
        * @param accessToken An OAuth access token from the Google SDK.
        * @param username A username used to create the user.
        * @param create True if the user should be created when authenticated.
        */
        virtual void authenticateGoogle(
            const std::string& accessToken,
            const std::string& username = std::string(),
            bool create = false,
            std::function<void(NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Authenticate a user with Apple Game Center.
         *
         * @param playerId The player id of the user in Game Center.
         * @param bundleId The bundle id of the Game Center application.
         * @param timestampSeconds The date and time that the signature was created.
         * @param salt A random <c>NSString</c> used to compute the hash and keep it randomized.
         * @param signature The verification signature data generated.
         * @param publicKeyUrl The URL for the public encryption key.
         * @param create True if the user should be created when authenticated.
         * @param username A username used to create the user.
         */
        virtual void authenticateGameCenter(
            const std::string& playerId,
            const std::string& bundleId,
            uint64_t timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            const std::string& username = std::string(),
            bool create = false,
            std::function<void(NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Authenticate a user with a custom id.
         *
         * @param id A custom identifier usually obtained from an external authentication service.
         * @param username A username used to create the user.
         * @param create True if the user should be created when authenticated.
         */
        virtual void authenticateCustom(
            const std::string& id,
            const std::string& username = std::string(),
            bool create = false,
            std::function<void(NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Authenticate a user with a Steam auth token.
         *
         * @param token An authentication token from the Steam network.
         * @param username A username used to create the user.
         * @param create True if the user should be created when authenticated.
         */
        virtual void authenticateSteam(
            const std::string& token,
            const std::string& username = std::string(),
            bool create = false,
            std::function<void(NSessionPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link a Facebook profile to a user account.
         *
         * @param session The session of the user.
         * @param accessToken An OAuth access token from the Facebook SDK.
         * @param importFriends True if the Facebook friends should be imported.
         */
        virtual void linkFacebook(
            NSessionPtr session,
            const std::string& accessToken,
            const opt::optional<bool>& importFriends = opt::nullopt,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link an email with password to the user account owned by the session.
         *
         * @param session The session of the user.
         * @param email The email address of the user.
         * @param password The password for the user.
         */
        virtual void linkEmail(
            NSessionPtr session,
            const std::string& email,
            const std::string& password,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link a device id to the user account owned by the session.
         *
         * @param session The session of the user.
         * @param id A device identifier usually obtained from a platform API.
         */
        virtual void linkDevice(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link a Google profile to a user account.
         *
         * @param session The session of the user.
         * @param accessToken An OAuth access token from the Google SDK.
         */
        virtual void linkGoogle(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link a Game Center profile to a user account.
         *
         * @param session The session of the user.
         * @param playerId The player id of the user in Game Center.
         * @param bundleId The bundle id of the Game Center application.
         * @param timestampSeconds The date and time that the signature was created.
         * @param salt A random <c>NSString</c> used to compute the hash and keep it randomized.
         * @param signature The verification signature data generated.
         * @param publicKeyUrl The URL for the public encryption key.
         */
        virtual void linkGameCenter(
            NSessionPtr session,
            const std::string& playerId,
            const std::string& bundleId,
            uint64_t timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link a Steam profile to a user account.
         *
         * @param session The session of the user.
         * @param token An authentication token from the Steam network.
         */
        virtual void linkSteam(
            NSessionPtr session,
            const std::string& token,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Link a custom id to the user account owned by the session.
         *
         * @param session The session of the user.
         * @param id A custom identifier usually obtained from an external authentication service.
         */
        virtual void linkCustom(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink a Facebook profile from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param accessToken An OAuth access token from the Facebook SDK.
         */
        virtual void unlinkFacebook(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink an email with password from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param email The email address of the user.
         * @param password The password for the user.
         */
        virtual void unlinkEmail(
            NSessionPtr session,
            const std::string& email,
            const std::string& password,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink a Google profile from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param accessToken An OAuth access token from the Google SDK.
         */
        virtual void unlinkGoogle(
            NSessionPtr session,
            const std::string& accessToken,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink a Game Center profile from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param playerId The player id of the user in Game Center.
         * @param bundleId The bundle id of the Game Center application.
         * @param timestampSeconds The date and time that the signature was created.
         * @param salt A random <c>NSString</c> used to compute the hash and keep it randomized.
         * @param signature The verification signature data generated.
         * @param publicKeyUrl The URL for the public encryption key.
         */
        virtual void unlinkGameCenter(
            NSessionPtr session,
            const std::string& playerId,
            const std::string& bundleId,
            uint64_t timestampSeconds,
            const std::string& salt,
            const std::string& signature,
            const std::string& publicKeyUrl,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink a Steam profile from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param token An authentication token from the Steam network.
         */
        virtual void unlinkSteam(
            NSessionPtr session,
            const std::string& token,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink a device id from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param id A device identifier usually obtained from a platform API.
         */
        virtual void unlinkDevice(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Unlink a custom id from the user account owned by the session.
         *
         * @param session The session of the user.
         * @param id A custom identifier usually obtained from an external authentication service.
         */
        virtual void unlinkCustom(
            NSessionPtr session,
            const std::string& id,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Import Facebook friends and add them to the user's account.
         *
         * The server will import friends when the user authenticates with Facebook. This function can be used to be
         * explicit with the import operation.
         *
         * @param session The session of the user.
         * @param token An OAuth access token from the Facebook SDK.
         * @param reset True if the Facebook friend import for the user should be reset.
         */
        virtual void importFacebookFriends(
            NSessionPtr session,
            const std::string& token,
            const opt::optional<bool>& reset = opt::nullopt,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Fetch the user account owned by the session.
         *
         * @param session The session of the user.
         */
        virtual void getAccount(
            NSessionPtr session,
            std::function<void(const NAccount&)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Update the current user's account on the server.
         *
         * @param session The session for the user.
         * @param username The new username for the user.
         * @param displayName A new display name for the user.
         * @param avatarUrl A new avatar url for the user.
         * @param langTag A new language tag in BCP-47 format for the user.
         * @param location A new location for the user.
         * @param timezone New timezone information for the user.
         */
        virtual void updateAccount(
            NSessionPtr session,
            const opt::optional<std::string>& username    = opt::nullopt,
            const opt::optional<std::string>& displayName = opt::nullopt,
            const opt::optional<std::string>& avatarUrl   = opt::nullopt,
            const opt::optional<std::string>& langTag     = opt::nullopt,
            const opt::optional<std::string>& location    = opt::nullopt,
            const opt::optional<std::string>& timezone    = opt::nullopt,
            std::function<void()> successCallback         = nullptr,
            ErrorCallback errorCallback                   = nullptr
        ) = 0;

        /**
        * Fetch one or more users by id, usernames, and Facebook ids.
        *
        * @param session The session of the user.
        * @param ids List of user IDs.
        * @param usernames List of usernames.
        * @param facebookIds List of Facebook IDs.
        */
        virtual void getUsers(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames = {},
            const std::vector<std::string>& facebookIds = {},
            std::function<void(const NUsers&)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Add one or more friends by id.
         *
         * @param session The session of the user.
         * @param ids The ids of the users to add or invite as friends.
         * @param usernames The usernames of the users to add as friends.
         */
        virtual void addFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames = {},
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Delete one more or users by id or username from friends.
         *
         * @param session The session of the user.
         * @param ids the user ids to remove as friends.
         * @param usernames The usernames to remove as friends.
         */
        virtual void deleteFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames = {},
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Block one or more friends by id.
         *
         * @param session The session of the user.
         * @param ids The ids of the users to block.
         * @param usernames The usernames of the users to block.
         */
        virtual void blockFriends(
            NSessionPtr session,
            const std::vector<std::string>& ids,
            const std::vector<std::string>& usernames = {},
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * List of friends of the current user.
        *
        * @param session The session of the user.
        */
        virtual void listFriends(
            NSessionPtr session,
            std::function<void(NFriendsPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Create a group.
         *
         * @param session The session of the user.
         * @param name The name for the group.
         * @param description A description for the group.
         * @param avatarUrl An avatar url for the group.
         * @param langTag A language tag in BCP-47 format for the group.
         * @param open True if the group should have open membership.
         */
        virtual void createGroup(
            NSessionPtr session,
            const std::string& name,
            const std::string& description = "",
            const std::string& avatarUrl = "",
            const std::string& langTag = "",
            bool open = false,
            std::function<void(const NGroup&)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Delete a group by id.
         *
         * @param session The session of the user.
         * @param groupId The group id to to remove.
         */
        virtual void deleteGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Add one or more users to the group.
         *
         * @param session The session of the user.
         * @param groupId The id of the group to add users into.
         * @param ids The ids of the users to add or invite to the group.
         */
        virtual void addGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * List all users part of the group.
        *
        * @param session The session of the user.
        * @param groupId The id of the group.
        */
        virtual void listGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void(NGroupUserListPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * Kick one or more users from the group.
        *
        * @param session The session of the user.
        * @param groupId The id of the group.
        * @param ids The ids of the users to kick.
        */
        virtual void kickGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * Join a group if it has open membership or request to join it.
        *
        * @param session The session of the user.
        * @param groupId The id of the group to join.
        */
        virtual void joinGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * Leave a group by id.
        *
        * @param session The session of the user.
        * @param groupId The id of the group to leave.
        */
        virtual void leaveGroup(
            NSessionPtr session,
            const std::string& groupId,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
        * List groups on the server.
        *
        * @param session The session of the user.
        * @param name The name filter to apply to the group list.
        * @param limit The number of groups to list.
        * @param cursor A cursor for the current position in the groups to list.
        */
        virtual void listGroups(
            NSessionPtr session,
            const std::string& name,
            int limit = 0,
            const std::string& cursor = "",
            std::function<void(NGroupListPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * List of groups the current user is a member of.
         *
         * @param session The session of the user.
         */
        virtual void listUserGroups(
            NSessionPtr session,
            std::function<void(NUserGroupListPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * List groups a user is a member of.
         *
         * @param session The session of the user.
         * @param userId The id of the user whose groups to list.
         */
        virtual void listUserGroups(
            NSessionPtr session,
            const std::string& userId,
            std::function<void(NUserGroupListPtr)> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Promote one or more users in the group.
         *
         * @param session The session of the user.
         * @param groupId The id of the group to promote users into.
         * @param ids The ids of the users to promote.
         */
        virtual void promoteGroupUsers(
            NSessionPtr session,
            const std::string& groupId,
            const std::vector<std::string>& ids,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;

        /**
         * Update a group.
         *
         * The user must have the correct access permissions for the group.
         *
         * @param session The session of the user.
         * @param groupId The id of the group to update.
         * @param name A new name for the group.
         * @param description A new description for the group.
         * @param avatarUrl A new avatar url for the group.
         * @param langTag A new language tag in BCP-47 format for the group.
         * @param open True if the group should have open membership.
         */
        virtual void updateGroup(
            NSessionPtr session,
            const std::string& groupId,
            const opt::optional<std::string> name = opt::nullopt,
            const opt::optional<std::string> description = opt::nullopt,
            const opt::optional<std::string> avatarUrl = opt::nullopt,
            const opt::optional<std::string> langTag = opt::nullopt,
            const opt::optional<bool> open = opt::nullopt,
            std::function<void()> successCallback = nullptr,
            ErrorCallback errorCallback = nullptr
        ) = 0;
    };

    using ClientPtr = std::shared_ptr<ClientInterface>;
}
