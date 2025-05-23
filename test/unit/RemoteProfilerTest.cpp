// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "aws/iotfleetwise/RemoteProfiler.h"
#include "SenderMock.h"
#include "WaitUntil.h"
#include "aws/iotfleetwise/IConnectionTypes.h"
#include "aws/iotfleetwise/LogLevel.h"
#include "aws/iotfleetwise/TopicConfig.h"
#include <atomic>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <string>
#include <vector>

namespace Aws
{
namespace IoTFleetWise
{

using ::testing::_;
using ::testing::Gt;
using ::testing::InvokeArgument;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::StrictMock;

void
checkMetrics( const std::string &data )
{
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE( reader.parse( data, root ) );
    ASSERT_GE( data.size(), 6 );
    ASSERT_GE( root["metric1"]["name"].asString().length(), 1 );
}

std::atomic<int> a( 0 );
TEST( RemoteProfilerTest, MetricsUpload )
{
    TopicConfigArgs topicConfigArgs;
    topicConfigArgs.metricsTopic = "metrics-topic";
    TopicConfig topicConfig( "thing-name", topicConfigArgs );
    StrictMock<Testing::SenderMock> senderMock( topicConfig );
    EXPECT_CALL( senderMock, mockedSendBuffer( "metrics-topic", _, Gt( 0 ), _ ) )
        .WillRepeatedly( InvokeArgument<3>( ConnectivityError::Success ) );

    RemoteProfiler profiler( senderMock, 1000, 1000, LogLevel::Trace, "Test" );
    profiler.start();

    // Generate some cpu load
    for ( int i = 0; i < 200000000; i++ )
    {
        a++;
    }

    WAIT_ASSERT_GT( senderMock.getSentBufferDataByTopic( "metrics-topic" ).size(), 5U );
    for ( auto sentData : senderMock.getSentBufferDataByTopic( "metrics-topic" ) )
    {
        ASSERT_NO_FATAL_FAILURE( checkMetrics( sentData.data ) );
    }
}

} // namespace IoTFleetWise
} // namespace Aws
