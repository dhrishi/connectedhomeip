/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements unit tests for CHIP Interaction Model Reporting Engine
 *
 */

#include <app/InteractionModelEngine.h>
#include <app/reporting/Engine.h>
#include <core/CHIPCore.h>
#include <core/CHIPTLV.h>
#include <core/CHIPTLVDebug.hpp>
#include <core/CHIPTLVUtilities.hpp>
#include <messaging/ExchangeContext.h>
#include <messaging/ExchangeMgr.h>
#include <messaging/Flags.h>
#include <platform/CHIPDeviceLayer.h>
#include <support/ErrorStr.h>
#include <support/UnitTestRegistration.h>
#include <system/SystemPacketBuffer.h>
#include <system/TLVPacketBufferBackingStore.h>
#include <transport/PASESession.h>
#include <transport/SecureSessionMgr.h>
#include <transport/raw/UDP.h>

#include <nlunit-test.h>

namespace chip {
static System::Layer gSystemLayer;
static SecureSessionMgr gSessionManager;
static Messaging::ExchangeManager gExchangeManager;
static TransportMgr<Transport::UDP> gTransportManager;
static const Transport::AdminId gAdminId = 0;

namespace app {
namespace reporting {
class TestReportingEngine
{
public:
    static void TestBuildAndSendSingleReportData(nlTestSuite * apSuite, void * apContext);
};

void TestReportingEngine::TestBuildAndSendSingleReportData(nlTestSuite * apSuite, void * apContext)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    app::ReadHandler readHandler;
    Engine reportingEngine;
    System::PacketBufferTLVWriter writer;
    System::PacketBufferHandle readRequestbuf = System::PacketBufferHandle::New(System::PacketBuffer::kMaxSize);
    ReadRequest::Builder readRequestBuilder;

    err = InteractionModelEngine::GetInstance()->Init(&gExchangeManager, nullptr);
    NL_TEST_ASSERT(apSuite, err == CHIP_NO_ERROR);
    Messaging::ExchangeContext * exchangeCtx = gExchangeManager.NewContext({ 0, 0, 0 }, nullptr);
    writer.Init(std::move(readRequestbuf));
    err = readRequestBuilder.Init(&writer);
    NL_TEST_ASSERT(apSuite, err == CHIP_NO_ERROR);
    readRequestBuilder.EventNumber(1);
    NL_TEST_ASSERT(apSuite, readRequestBuilder.GetError() == CHIP_NO_ERROR);
    readRequestBuilder.EndOfReadRequest();
    NL_TEST_ASSERT(apSuite, readRequestBuilder.GetError() == CHIP_NO_ERROR);
    err = writer.Finalize(&readRequestbuf);
    NL_TEST_ASSERT(apSuite, err == CHIP_NO_ERROR);

    readHandler.OnReadRequest(exchangeCtx, std::move(readRequestbuf));
    reportingEngine.Init();
    err = reportingEngine.BuildAndSendSingleReportData(&readHandler);
    NL_TEST_ASSERT(apSuite, err == CHIP_ERROR_NOT_CONNECTED);
}
} // namespace reporting
} // namespace app
} // namespace chip

namespace {
void InitializeChip(nlTestSuite * apSuite)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    chip::Optional<chip::Transport::PeerAddress> peer(chip::Transport::Type::kUndefined);
    chip::Transport::AdminPairingTable admins;
    chip::Transport::AdminPairingInfo * adminInfo = admins.AssignAdminId(chip::gAdminId, chip::kTestDeviceNodeId);

    NL_TEST_ASSERT(apSuite, adminInfo != nullptr);

    err = chip::Platform::MemoryInit();
    NL_TEST_ASSERT(apSuite, err == CHIP_NO_ERROR);

    chip::gSystemLayer.Init(nullptr);

    err = chip::gSessionManager.Init(chip::kTestDeviceNodeId, &chip::gSystemLayer, &chip::gTransportManager, &admins);
    NL_TEST_ASSERT(apSuite, err == CHIP_NO_ERROR);

    err = chip::gExchangeManager.Init(&chip::gSessionManager);
    NL_TEST_ASSERT(apSuite, err == CHIP_NO_ERROR);
}

// clang-format off
const nlTest sTests[] =
        {
                NL_TEST_DEF("CheckBuildAndSendSingleReportData", chip::app::reporting::TestReportingEngine::TestBuildAndSendSingleReportData),
                NL_TEST_SENTINEL()
        };
// clang-format on
} // namespace

int TestReportingEngine()
{
    // clang-format off
    nlTestSuite theSuite =
	{
        "TestReportingEngine",
        &sTests[0],
        nullptr,
        nullptr
    };
    // clang-format on

    InitializeChip(&theSuite);

    nlTestRunner(&theSuite, nullptr);

    return (nlTestRunnerStats(&theSuite));
}

CHIP_REGISTER_TEST_SUITE(TestReportingEngine)