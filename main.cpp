#include <iostream>
#include "httpClient/httpProvider.h"
#include "httpClient/pal.h"
#include "nakama-cpp/log/NLogger.h"

#include <httpClient/httpClient.h>

bool g_waitHc = false;
bool g_waitNl = false;

void HttpCallback(XAsyncBlock* async) {
    HCCallHandle call = reinterpret_cast<HCCallHandle>(async->context);
    uint32_t statusCode;
    const char* responseBody;

    HCHttpCallResponseGetStatusCode(call, &statusCode);
    HCHttpCallResponseGetResponseString(call, &responseBody);

    std::cout << "HTTP Status: " << statusCode << std::endl;
    std::cout << "Response: " << responseBody << std::endl;

    // Clean up
    HCHttpCallCloseHandle(call);
    g_waitHc = false;
}

void testHc() {
    g_waitHc = true;

    HCInitialize(nullptr);
    XAsyncBlock async = {};
    async.callback = HttpCallback;

    // Create an HTTP call
    HCCallHandle call;
    HCHttpCallCreate(&call);
    async.context = call;
    HCHttpCallRequestSetUrl(call, "GET", "https://dev.gabrielsulka.se/healthcheck?http_key=defaultkey");
    // 192.168.0.173
    //  Nakama::NClientParameters parameters;
    //  parameters.serverKey = "defaultkey";
    //  parameters.host = "127.0.0.1";
    //  parameters.port = 7350;
    //  Perform the async request
    HCHttpCallPerformAsync(call, &async);

    // Wait for the user to press Enter (so the program doesn't exit immediately)
    while (g_waitHc)
        ;

    // Shutdown libHttpClient
    HCCleanup();
}

#include <nakama-cpp/ClientFactory.h>

void testNl() {
    g_waitNl = true;
    Nakama::NLogger::initWithConsoleSink(Nakama::NLogLevel::Debug);
    Nakama::NClientParameters parameters;
    parameters.serverKey = "defaultkey";
    parameters.host = "dev.gabrielsulka.se";
    // parameters.port = 7350;
    parameters.ssl = true;

    auto transport = createDefaultHttpTransport(parameters.platformParams);
    auto client = Nakama::createDefaultClient(parameters);

    auto successCallback = [](Nakama::NSessionPtr session) {
        std::cout << "session token: " << session->getAuthToken() << std::endl;
        g_waitNl = false;
    };

    auto errorCallback = [](const Nakama::NError& error) {
        std::cout << "error:" << error.message << std::endl;
        g_waitNl = false;
    };

    client->authenticateDevice("70d396db-2cd2-4223-a79e-7c5abfb1479f", "Hello123", true, {}, successCallback, errorCallback);

    while (g_waitNl)
        client->tick();
    ;
}

int main() {
    // testHc();
    testNl();

    return 0;
}