/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
 * @file DeviceCallbacks.cpp
 *
 * Implements all the callbacks to the application from the CHIP Stack
 *
 **/
#include "DeviceCallbacks.h"

#include "CHIPDeviceManager.h"
#include "Globals.h"
#include "LEDWidget.h"
#include "WiFiWidget.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "light_driver.h"
#include "gen/attribute-id.h"
#include "gen/cluster-id.h"
#include <app/util/basic-types.h>
#include <lib/mdns/DiscoveryManager.h>
#include <support/CodeUtils.h>
#include "app_priv.h"

static const char * TAG = "echo-devicecallbacks";

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::DeviceLayer;

uint32_t identifyTimerCount;
constexpr uint32_t kIdentifyTimerDelayMS = 250;

void DeviceCallbacks::DeviceEventCallback(const ChipDeviceEvent * event, intptr_t arg)
{
    switch (event->Type)
    {
    case DeviceEventType::kInternetConnectivityChange:
        OnInternetConnectivityChange(event);
        break;

    case DeviceEventType::kSessionEstablished:
        OnSessionEstablished(event);
        break;
    }

    ESP_LOGI(TAG, "Current free heap: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

void DeviceCallbacks::PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId, uint8_t mask,
                                                  uint16_t manufacturerCode, uint8_t type, uint8_t size, uint8_t * value)
{
    ESP_LOGD(TAG, "PostAttributeChangeCallback - Cluster ID: '0x%04x', EndPoint ID: '0x%02x', Attribute ID: '0x%04x'", clusterId,
             endpointId, attributeId);

    switch (clusterId)
    {
    case ZCL_ON_OFF_CLUSTER_ID:
        OnOnOffPostAttributeChangeCallback(endpointId, attributeId, value);
        break;
    case ZCL_COLOR_CONTROL_CLUSTER_ID:
        OnColorControlPostAttributeChangeCallback(endpointId, attributeId, size, value);
        break;
    case ZCL_LEVEL_CONTROL_CLUSTER_ID:
        OnLevelPostAttributeChangeCallback(endpointId, attributeId, size, value);
        break;
    case ZCL_IDENTIFY_CLUSTER_ID:
        OnIdentifyPostAttributeChangeCallback(endpointId, attributeId, value);
        break;

    default:
        ESP_LOGW(TAG, "Unhandled cluster ID: %d", clusterId);
        break;
    }

    ESP_LOGI(TAG, "Current free heap: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

void DeviceCallbacks::OnInternetConnectivityChange(const ChipDeviceEvent * event)
{
    if (event->InternetConnectivityChange.IPv4 == kConnectivity_Established)
    {
        ESP_LOGI(TAG, "Server ready at: %s:%d", event->InternetConnectivityChange.address, CHIP_PORT);
        wifiLED.Set(true);
        chip::Mdns::DiscoveryManager::GetInstance().StartPublishDevice();
    }
    else if (event->InternetConnectivityChange.IPv4 == kConnectivity_Lost)
    {
        ESP_LOGE(TAG, "Lost IPv4 connectivity...");
        wifiLED.Set(false);
        app_light_indicate_wifi_fail();
    }
    if (event->InternetConnectivityChange.IPv6 == kConnectivity_Established)
    {
        ESP_LOGI(TAG, "IPv6 Server ready...");
        chip::Mdns::DiscoveryManager::GetInstance().StartPublishDevice();
    }
    else if (event->InternetConnectivityChange.IPv6 == kConnectivity_Lost)
    {
        ESP_LOGE(TAG, "Lost IPv6 connectivity...");
    }
}

void DeviceCallbacks::OnSessionEstablished(const ChipDeviceEvent * event)
{
    if (event->SessionEstablished.IsCommissioner)
    {
        ESP_LOGI(TAG, "Commissioner detected!");
    }
}

void DeviceCallbacks::OnOnOffPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    VerifyOrExit(attributeId == ZCL_ON_OFF_ATTRIBUTE_ID, ESP_LOGI(TAG, "Unhandled Attribute ID: '0x%04x", attributeId));
    VerifyOrExit(endpointId == 1 || endpointId == 2, ESP_LOGE(TAG, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    // At this point we can assume that value points to a bool value.
    if (endpointId == 1)
    {
        app_light_set_switch(*value);
        statusLED1.Set(*value);
    }
    else
    {
        statusLED2.Set(*value);
    }
exit:
    return;
}

void DeviceCallbacks::OnColorControlPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t size, uint8_t * value)
{
    // At this point we can assume that value points to a bool value.
    uint16_t hue_degrees;
    uint8_t saturation_percentage;
    VerifyOrExit(endpointId == 1, ESP_LOGE(TAG, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    ESP_LOGD(TAG, "In color control callback");
    switch (attributeId) {
    case ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID:
        hue_degrees = value[0] * 360 / 254;
        ESP_LOGD(TAG, "Setting hue %u degrees on the light", hue_degrees);
        light_driver_set_hue(hue_degrees);
        light_driver_hsv2rgb(light_driver_get_hue(), light_driver_get_saturation(), light_driver_get_value(),
                &g_red, &g_green, &g_blue);
        break;
    case ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID:
        saturation_percentage = value[0] * 100 / 254;
        ESP_LOGD(TAG, "Setting saturation %u %% on the light", saturation_percentage);
        light_driver_set_saturation(saturation_percentage);
        light_driver_hsv2rgb(light_driver_get_hue(), light_driver_get_saturation(), light_driver_get_value(),
                &g_red, &g_green, &g_blue);
        break;
    default:
        ESP_LOGW(TAG, "Unhandled attribute -- 0x%x", attributeId);
    }
exit:
    return;
}

void DeviceCallbacks::OnLevelPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t size, uint8_t * value)
{
    // At this point we can assume that value points to a bool value.
    uint8_t brightness_percentage;
    VerifyOrExit(endpointId == 1, ESP_LOGE(TAG, "Unexpected EndPoint ID: `0x%02x'", endpointId));
    
    ESP_LOGD(TAG, "In level control callback");

    /* Need to handle ZCL_ON_LEVEL_ATTRIBUTE_ID as well */
    /* Need to define ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_LEVEL_ATTRIBUTE */
    switch (attributeId) {
    case ZCL_CURRENT_LEVEL_ATTRIBUTE_ID:
        brightness_percentage = value[0] * 100 / 255;
        ESP_LOGD(TAG, "Setting brightness %u %% on the light", brightness_percentage);
        light_driver_set_brightness(brightness_percentage);
        break;
    default:
        ESP_LOGW(TAG, "Unhandled attribute -- 0x%x", attributeId);
    }
exit:
    return;
}

void IdentifyTimerHandler(Layer * systemLayer, void * appState, Error error)
{
    statusLED1.Animate();

    // Decrement the timer count.
    identifyTimerCount--;

    if (identifyTimerCount)
    {
        SystemLayer.StartTimer(kIdentifyTimerDelayMS, IdentifyTimerHandler, appState);
    }
}

void DeviceCallbacks::OnIdentifyPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    VerifyOrExit(attributeId == ZCL_IDENTIFY_TIME_ATTRIBUTE_ID, ESP_LOGI(TAG, "Unhandled Attribute ID: '0x%04x", attributeId));
    VerifyOrExit(endpointId == 1, ESP_LOGE(TAG, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    statusLED1.Blink(kIdentifyTimerDelayMS * 2);

    // timerCount represents the number of callback execution before the we stopped the timer.
    // value is expressed in seconds and the timer is fired every 250ms, so just multiply value by 4.
    // Also, we want timerCount to be odd number, so the ligth state ends in the same state it starts.
    identifyTimerCount = (*value) * 4;

    SystemLayer.CancelTimer(IdentifyTimerHandler, this);
    SystemLayer.StartTimer(kIdentifyTimerDelayMS, IdentifyTimerHandler, this);

exit:
    return;
}
