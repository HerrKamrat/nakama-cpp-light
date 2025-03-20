cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(nakama-light)

set(HTTP_CLIENT_SRC
    libHttpClient/Source/WebSocket/hcwebsocket.h
    libHttpClient/Source/WebSocket/websocket_publics.cpp
    libHttpClient/Source/WebSocket/Generic/generic_websocket.cpp
    libHttpClient/Source/WebSocket/hcwebsocket.cpp
    libHttpClient/Source/Logger/log_publics.cpp
    libHttpClient/Source/Logger/trace.cpp
    libHttpClient/Source/Logger/trace_internal.h
    libHttpClient/Source/Mock/lhc_mock.cpp
    libHttpClient/Source/Mock/lhc_mock.h
    libHttpClient/Source/Mock/mock_publics.cpp
    libHttpClient/Source/Platform/IWebSocketProvider.h
    libHttpClient/Source/Platform/ExternalWebSocketProvider.cpp
    libHttpClient/Source/Platform/IHttpProvider.cpp
    libHttpClient/Source/Platform/ExternalHttpProvider.cpp

    libHttpClient/Source/Platform/ExternalHttpProvider.h
    libHttpClient/Source/Platform/PlatformComponents.h
    libHttpClient/Source/Platform/ExternalWebSocketProvider.h

    libHttpClient/Source/Platform/IHttpProvider.h
    libHttpClient/Source/Platform/PlatformTrace.h
    libHttpClient/Source/Common/utils.h
    libHttpClient/Source/Common/utils.cpp
    libHttpClient/Source/Common/pch.h
    libHttpClient/Source/Common/Types.h
    libHttpClient/Source/Common/uri.h
    libHttpClient/Source/Common/buildver.h
    libHttpClient/Source/Common/Result.h
    libHttpClient/Source/Common/pal_internal.h
    libHttpClient/Source/Common/uri.cpp
    libHttpClient/Source/Common/EntryList.h
    libHttpClient/Source/Common/pch.cpp
    libHttpClient/Source/Common/pch_common.h
    libHttpClient/Source/Common/ResultMacros.h
    libHttpClient/Source/HTTP/httpcall_response.cpp
    libHttpClient/Source/HTTP/compression.h
    libHttpClient/Source/HTTP/compression.cpp
    libHttpClient/Source/HTTP/httpcall.h

    libHttpClient/Source/HTTP/httpcall_publics.cpp
    libHttpClient/Source/HTTP/httpcall_request.cpp

    libHttpClient/Source/HTTP/httpcall.cpp
    libHttpClient/Source/Task/XAsyncProviderPriv.h
    libHttpClient/Source/Task/ThreadPool.h
    libHttpClient/Source/Task/TaskQueueP.h
    libHttpClient/Source/Task/LocklessQueue.h
    libHttpClient/Source/Task/TaskQueueImpl.h
    libHttpClient/Source/Task/ThreadPool_stl.cpp

    libHttpClient/Source/Task/WaitTimer_stl.cpp
    libHttpClient/Source/Task/AtomicVector.h
    libHttpClient/Source/Task/WaitTimer.h
    libHttpClient/Source/Task/StaticArray.h
    libHttpClient/Source/Task/XTaskQueuePriv.h
    libHttpClient/Source/Task/TaskQueue.cpp
    libHttpClient/Source/Task/referenced_ptr.h
    libHttpClient/Source/Task/AsyncLib.cpp
    libHttpClient/Source/Global/global.h
    libHttpClient/Source/Global/NetworkState.cpp
    libHttpClient/Source/Global/NetworkState.h
    libHttpClient/Source/Global/global_publics.cpp
    libHttpClient/Source/Global/mem.h
    libHttpClient/Source/Global/global.cpp
    libHttpClient/Source/Global/mem.cpp
)

set(HTTP_CLIENT_HDR
    libHttpClient/Include

    libHttpClient/Source
    libHttpClient/Source/WebSocket

    libHttpClient/Source/SSL
    libHttpClient/Source/Logger
    libHttpClient/Source/Mock
    libHttpClient/Source/Platform
    libHttpClient/Source/Platform/Apple
    libHttpClient/Source/Platform/Generic
    libHttpClient/Source/Common
    libHttpClient/Source/Common/Apple
    libHttpClient/Source/HTTP
    libHttpClient/Source/HTTP/Apple
    libHttpClient/Source/HTTP/Generic
    libHttpClient/Source/Task
    libHttpClient/Source/Global
)

if(APPLE)
    list(APPEND HTTP_CLIENT_SRC
        libHttpClient/Source/Platform/Apple/PlatformComponents_Apple.cpp
        libHttpClient/Source/Platform/Apple/PlatformTrace_Apple.cpp
        libHttpClient/Source/Common/Apple/utils_apple.h
        libHttpClient/Source/Common/Apple/utils_apple.mm
        libHttpClient/Source/HTTP/Apple/http_apple.mm
        libHttpClient/Source/HTTP/Apple/session_delegate.mm
        libHttpClient/Source/HTTP/Apple/session_delegate.h
        libHttpClient/Source/HTTP/Apple/http_apple.h
        libHttpClient/Source/HTTP/Apple/request_body_stream.h
        libHttpClient/Source/HTTP/Apple/request_body_stream.mm
    )
else()

    list(APPEND HTTP_CLIENT_SRC
        libHttpClient/Source/HTTP/Android/android_http_request.cpp
        libHttpClient/Source/HTTP/Android/android_http_request.h
        libHttpClient/Source/HTTP/Android/http_android.cpp
        libHttpClient/Source/HTTP/Android/http_android.h
        libHttpClient/Source/WebSocket/Android/AndroidWebSocketProvider.cpp
        libHttpClient/Source/WebSocket/Android/AndroidWebSocketProvider.h
        libHttpClient/Source/Platform/Android/PlatformComponents_Android.cpp
        libHttpClient/Source/Platform/Android/PlatformTrace_Android.cpp
        libHttpClient/Source/Platform/Android/PlatformComponents_Android.h
        libHttpClient/Source/HTTP/Android/android_http_request.cpp
        libHttpClient/Source/HTTP/Android/AndroidHttpProvider.cpp
        libHttpClient/Source/HTTP/Android/AndroidHttpProvider.h
        libHttpClient/Source/HTTP/Android/android_http_request.h
        libHttpClient/Source/HTTP/Android/http_android.cpp
        libHttpClient/Source/HTTP/Android/http_android.h
    )
endif()

set(NAKAMA_SRC

    impl/httpLibHttpClient/NHttpClientLibHC.h
    impl/httpLibHttpClient/NHttpClientLibHC.cpp

    # interface/include/nakama-cpp/ClientFactory.h
    # interface/include/nakama-cpp/URLParts.h
    # interface/include/nakama-cpp/NakamaVersion.h
    # interface/include/nakama-cpp/Nakama.h
    # interface/include/nakama-cpp/NTypes.h
    # interface/include/nakama-cpp/NError.h
    # interface/include/nakama-cpp/NUtils.h
    # interface/include/nakama-cpp/NSessionInterface.h
    # interface/include/nakama-cpp/NHttpTransportInterface.h
    # interface/include/nakama-cpp/NExport.h

    # interface/include/nakama-cpp/NClientInterface.h
    # interface/include/nakama-cpp/log/NConsoleLogSink.h
    # interface/include/nakama-cpp/log/NLogger.h
    # interface/include/nakama-cpp/log/NLogSinkInterface.h
    # interface/include/nakama-cpp/NPlatformParams.h
    # interface/include/nakama-cpp/data/NGroupUserList.h
    # interface/include/nakama-cpp/data/NLeaderboardRecord.h
    # interface/include/nakama-cpp/data/NNotificationList.h
    # interface/include/nakama-cpp/data/NTournamentRecordList.h
    # interface/include/nakama-cpp/data/NAccountDevice.h
    # interface/include/nakama-cpp/data/NChannelMessage.h
    # interface/include/nakama-cpp/data/NUsers.h
    # interface/include/nakama-cpp/data/NGroupUser.h
    # interface/include/nakama-cpp/data/NGroupList.h
    # interface/include/nakama-cpp/data/NAccount.h
    # interface/include/nakama-cpp/data/NFriend.h
    # interface/include/nakama-cpp/data/NStorageObjectWrite.h
    # interface/include/nakama-cpp/data/NStorageObjectAck.h
    # interface/include/nakama-cpp/data/NUserGroupList.h
    # interface/include/nakama-cpp/data/NRpc.h
    # interface/include/nakama-cpp/data/NStorageObjectId.h
    # interface/include/nakama-cpp/data/NChannelMessageList.h
    # interface/include/nakama-cpp/data/NMatchList.h
    # interface/include/nakama-cpp/data/NUser.h
    # interface/include/nakama-cpp/data/NTournament.h
    # interface/include/nakama-cpp/data/NTournamentList.h
    # interface/include/nakama-cpp/data/NNotification.h
    # interface/include/nakama-cpp/data/NStorageObjectList.h
    # interface/include/nakama-cpp/data/NFriendList.h
    # interface/include/nakama-cpp/data/NStoragePermissions.h
    # interface/include/nakama-cpp/data/NLeaderboardRecordList.h
    # interface/include/nakama-cpp/data/NUserGroup.h
    # interface/include/nakama-cpp/data/NGroup.h
    # interface/include/nakama-cpp/data/NStorageObject.h
    # interface/include/nakama-cpp/data/NMatch.h
    # interface/include/nakama-cpp/NException.h

    core/common/DefaultSession.cpp
    core/common/NError.cpp
    core/common/NRtError.cpp
    core/common/DefaultSession.h
    core/common/NUtils.cpp
    # core/common/DataHelper.cpp
    # core/common/RapidjsonHelper.h
    core/common/StrUtil.cpp
    core/common/BaseClient.h
    core/common/BaseClient.cpp
    core/common/StrUtil.h
    core/common/Version.cpp
    # core/common/DataHelper.h
    core/core-rest/RestClient.h
    # core/core-rest/grpc_status_code_enum.h
    core/core-rest/RestClient.cpp
    # core/src/roots_pem.cpp
    core/src/log/NLogger.cpp
    core/src/log/NConsoleLogSink.cpp
    # core/src/roots_pem.h

    factory/ClientFactory.cpp
    # factory/NWebsocketsFactory.cpp

    # core/core-rest/LightRestClient.cpp
)

add_library(libhttpclient ${HTTP_CLIENT_SRC})
target_include_directories(libhttpclient PUBLIC ${HTTP_CLIENT_HDR})
target_compile_definitions(libhttpclient PUBLIC 
    HC_LINK_STATIC
    HC_NOWEBSOCKETS)

add_library(nakama-light ${NAKAMA_SRC})
target_include_directories(nakama-light PUBLIC 
    interface/include
    external_include/
    include/
)

target_include_directories(nakama-light PRIVATE 
    core/common/
    core/
)

target_compile_definitions(nakama-light PUBLIC 
    NLOGS_ENABLED
    WITH_EXTERNAL_WS
    BUILD_IO_EXTERNAL
    BUILD_HTTP_LIBHTTPCLIENT
    BUILD_IO_EXTERNAL
    )

target_link_libraries(nakama-light libhttpclient)


if(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_package(ZLIB REQUIRED)

    target_link_libraries(nakama-light ${COCOA_LIBRARY})
    target_link_libraries(nakama-light ZLIB::ZLIB)
    target_compile_options(libhttpclient PUBLIC "-fobjc-arc")

elseif(ANDROID)
    target_link_libraries(nakama-light log z)
endif()


if(PROJECT_IS_TOP_LEVEL)
    add_executable(HelloWorld main.cpp)
    target_link_libraries(HelloWorld nakama-light)
endif()