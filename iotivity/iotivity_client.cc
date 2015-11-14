/*
 * Copyright (c) 2015 Cisco and/or its affiliates. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Cisco nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string>
#include <map>
#include <algorithm>

#include "iotivity/iotivity_client.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_resource.h"


IotivityClient::IotivityClient(IotivityDevice *device) : m_device(device) {
}

IotivityClient::~IotivityClient() {
  for (auto const &entity : m_resourcemap) {
    IotivityResourceClient *resClient = entity.second;
    delete resClient;
  }
  m_resourcemap.clear();
  m_foundresourcemap.clear();
}

void IotivityClient::foundResourceCallback(std::shared_ptr<OCResource> resource,
    const picojson::value& value) {
  DEBUG_MSG("\n###foundResourceCallback:\n");
  picojson::value param = value.get("OicDiscoveryOptions");
  std::string deviceId = "";

  if (param.contains("deviceId")) {
    deviceId = param.get("deviceId").to_str();
  }

  IotivityResourceClient *resClient = new IotivityResourceClient(m_device);
  resClient->setSharedPtr(resource);
  PrintfOcResource((const OCResource &)*resource);

  if (deviceId == "" || resource->sid() == deviceId) {
    m_foundresourcemap[resClient->getResourceId()] = resClient;
    // No collisions between sid and resource
    m_foundresourcemap[resource->sid()] = resClient;
  }

  int waitsec = GetWait(value);

  if (waitsec == -1) {
    findPreparedRequest(value);
  }
}

void IotivityClient::findDevicePreparedRequest(const picojson::value& value) {
  DEBUG_MSG("findDevicePreparedRequest\n");

  std::lock_guard<std::mutex> lock(m_callbackLockDevices);
  picojson::value::object object;
  object["cmd"] = picojson::value("findDevicesCompleted");

  double async_call_id = value.get("asyncCallId").get<double>();
  object["asyncCallId"] = picojson::value(async_call_id);
  picojson::array devicesArray;

  for (auto const &entity : m_founddevicemap) {
    IotivityDeviceInfo *deviceInfo = entity.second;
    picojson::object deviceobj;
    picojson::object properties;
    deviceInfo->serialize(properties);
    deviceobj["info"] = picojson::value(properties);
    devicesArray.push_back(picojson::value(deviceobj));
    m_devicemap[entity.first] = deviceInfo;
  }

  object["devicesArray"] = picojson::value(devicesArray);
  m_device->PostMessage(picojson::value(object).serialize().c_str());
}

void IotivityClient::findDeviceTimerCallback(const picojson::value& value) {
  int waitsec = GetWait(value);

  sleep(waitsec);
  findDevicePreparedRequest(value);
}

void IotivityClient::findPreparedRequest(const picojson::value &value) {
  std::lock_guard<std::mutex> lock(m_callbackLock);
  picojson::value::object object;
  object["cmd"] = picojson::value("foundResourceCallback");

  double async_call_id = value.get("asyncCallId").get<double>();
  object["asyncCallId"] = picojson::value(async_call_id);
  picojson::array resourcesArray;

  for (auto const &entity : m_foundresourcemap) {
    IotivityResourceClient *resClient = entity.second;
    picojson::object resourceobj;
    resClient->serialize(resourceobj);
    picojson::value v = picojson::value(resourceobj);

    // no duplicates
    if (std::find(resourcesArray.begin(),
                  resourcesArray.end(), v) == resourcesArray.end()) {
      resourcesArray.push_back(v);
    }
    m_resourcemap[entity.first] = entity.second;
  }

  object["resourcesArray"] = picojson::value(resourcesArray);
  m_device->PostMessage(picojson::value(object).serialize().c_str());
}

void IotivityClient::findResourceTimerCallback(const picojson::value &value) {
  int waitsec = GetWait(value);

  sleep(waitsec);
  findPreparedRequest(value);
}

void IotivityClient::foundDeviceCallback(const OCRepresentation &rep,
    const picojson::value &value) {
  DEBUG_MSG("\n###foundDeviceCallback:\n");

  std::string val;
  std::string values[] = {
    "di", "Device ID        ", "n", "Device name      ", "lcv",
    "Spec version url ", "dmv", "Data Model Model ",
  };

  for (unsigned int i = 0; i < sizeof(values) / sizeof(values[0]); i += 2) {
    if (rep.getValue(values[i], val)) {
      DEBUG_MSG("\t%s:%s\n", values[i + 1].c_str(), val.c_str());
    }
  }

  PrintfOcRepresentation(rep);

  // Populate device info if "di"
  if (rep.getValue("di", val)) {
    std::string deviceUUID = val;

    IotivityDeviceInfo *deviceInfo = NULL;
    std::map<std::string, IotivityDeviceInfo *>::const_iterator it;
    it = m_founddevicemap.find(deviceUUID);

    if (it != m_founddevicemap.end()) {
      deviceInfo = m_founddevicemap[deviceUUID];
    } else {
      deviceInfo = new IotivityDeviceInfo();
    }

    deviceInfo->m_deviceinfomap["uuid"] = deviceUUID;

    if (rep.getValue("n", val)) { deviceInfo->m_deviceinfomap["name"] = val; }
    if (rep.getValue("lcv", val)) {
      deviceInfo->m_deviceinfomap["coreSpecVersion"] = val;
    }
    if (rep.getValue("dmv", val)) {
      deviceInfo->m_deviceinfomap["dataModels"] = val;
    }
    m_founddevicemap[deviceUUID] = deviceInfo;
  } else {
    // if no "di" shoudl not happen. Post immediately
    int waitsec = GetWait(value);

    if (waitsec == -1) {
      findDevicePreparedRequest(value);
    }
  }
}

void IotivityClient::foundPlatformByDeviceCallback(const OCRepresentation &rep,
    std::string deviceId,
    const picojson::value &value) {
  // Request was done with host, so should only get one cb
  DEBUG_MSG("\n###foundPlatformDeviceCallback deviceId = %s\n",
            deviceId.c_str());

  std::string val;
  std::string values[] = {
    "pi",   "Platform ID                    ", "platformId",
    "mndt", "Manufactured Date              ", "manufacturerDate",
    "mnhw", "Manufacturer hardware version  ", "hardwareVersion",
    "mnml", "Manufacturer url               ", "manufacturerUrl",
    "mnmn", "Manufacturer name              ", "manufacturerName",
    "mnmo", "Manufacturer Model No          ", "model",
    "mnos", "Manufacturer OS version        ", "osVersion",
    "mnpv", "Manufacturer Platform Version  ", "platfromeVersion",
    "mnfv", "Manufacturer firmware version  ", "firmwareVersion",
    "mnsl", "Manufacturer support url       ", "supportUrl",
    "st",   "Manufacturer system time       ", "systemTime"
  };

  PrintfOcRepresentation(rep);
  std::string deviceUUID = deviceId;

  IotivityDeviceInfo *deviceInfo = NULL;
  std::map<std::string, IotivityDeviceInfo *>::const_iterator it;
  it = m_founddevicemap.find(deviceUUID);

  if (it != m_founddevicemap.end()) {
    DEBUG_MSG("\t Use EXISTING info for uuid %s\n", deviceUUID.c_str());
    deviceInfo = m_founddevicemap[deviceUUID];
  } else {
    DEBUG_MSG("\t######### ERROR %s\n", deviceUUID.c_str());
    return;
  }

  for (unsigned int i = 0; i < sizeof(values) / sizeof(values[0]); i += 3) {
    if (rep.getValue(values[i], val)) {
      deviceInfo->m_deviceinfomap[values[i + 2]] = val;
      DEBUG_MSG("\t Device info[%s] = :%s\n",
                values[i + 2].c_str(),
                val.c_str());
    }
  }

  int waitsec = GetWait(value);

  if (waitsec == -1) {
    findDevicePreparedRequest(value);
  }
}


IotivityResourceClient *IotivityClient::getResourceByDeviceId(
  std::string deviceId) {
  // all in same map
  return getResourceById(deviceId);
}

IotivityResourceClient *IotivityClient::getResourceById(std::string id) {
  if (m_resourcemap.size()) {
    std::map<std::string, IotivityResourceClient *>::const_iterator it;
    if ((it = m_resourcemap.find(id)) != m_resourcemap.end()) {
      return (*it).second;
    }
  }

  return NULL;
}

void IotivityClient::handleCancelObserving(const picojson::value &value) {
  DEBUG_MSG("handleCancelObserving: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  std::string resId = value.get("id").to_str();
  IotivityResourceClient *resClient = getResourceById(resId);

  if (resClient != NULL) {
    OCStackResult result = resClient->cancelObserving(async_call_id);
    if (OC_STACK_OK != result) {
      m_device->postError(async_call_id);
      return;
    }
  } else {
    m_device->postError(async_call_id);
    return;
  }
  m_device->postResult("cancelObservingCompleted", async_call_id);
}

void IotivityClient::handleCreateResource(const picojson::value &value) {
  // Post + particular data
  DEBUG_MSG("handleCreateResource: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  IotivityResourceInit oicResourceInit(value.get("OicResourceInit"));
  std::string resId = value.get("id").to_str();
  IotivityResourceClient *resClient = getResourceById(resId);

  if (resClient != NULL) {
    OCStackResult result =
      resClient->createResource(oicResourceInit, async_call_id);
    if (OC_STACK_OK != result) {
      m_device->postError(async_call_id);
      return;
    }
  } else {
    m_device->postError(async_call_id);
  }
}

void IotivityClient::handleDeleteResource(const picojson::value &value) {
  DEBUG_MSG("handleDeleteResource: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  std::string resId = value.get("id").to_str();
  IotivityResourceClient *resClient = getResourceById(resId);

  if (resClient != NULL) {
    OCStackResult result = resClient->deleteResource(async_call_id);
    if (OC_STACK_OK != result) {
      m_device->postError(async_call_id);
      return;
    }
  } else {
    m_device->postError(async_call_id);
  }
}

void IotivityClient::handleFindDevices(const picojson::value &value) {
  DEBUG_MSG("handleFindDevices: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  picojson::value param = value.get("OicDiscoveryOptions");

  // all properties are null by default, meaning “find all”
  // if resourceId is specified in full form, a direct retrieve is made
  // if resourceType is specified, a retrieve on /oic/res is made
  // if resourceId is null, and deviceId not,
  // then only resources from that device are returned
  std::string deviceId = "";
  std::string resourceId = "";
  std::string resourceType = "";
  int waitsec = GetWait(value);

  if (param.contains("deviceId")) {
    deviceId = param.get("deviceId").to_str();
  }

  if (param.contains("resourceId")) {
    resourceId = param.get("resourceId").to_str();
  }

  if (param.contains("resourceType")) {
    resourceType = param.get("resourceType").to_str();
  }

  DEBUG_MSG(
    "###handleFindDevices: device = %s\n"
    "\tresource = %s\n"
    "\tresourceType = %s\n"
    "\ttimeout = %d\n",
    deviceId.c_str(), resourceId.c_str(), resourceType.c_str(), waitsec);

  DEBUG_MSG("will  clear devicemap 1\n");
  for (auto const &entity : m_founddevicemap) {
    IotivityDeviceInfo *deviceInfo = entity.second;
    delete deviceInfo;
  }

  m_founddevicemap.clear();

  std::string hostUri = "";  //  multicast
  std::string deviceDiscoveryURI = "/oic/d";
  std::string deviceDiscoveryRequest = OC_MULTICAST_PREFIX +
                                       deviceDiscoveryURI;

  DEBUG_MSG("process: hostUri=%s, uri1=%s, timeout=%ds\n",
            hostUri.c_str(), deviceDiscoveryRequest.c_str(), waitsec);

  FindDeviceCallback deviceInfoHandler =
    std::bind(&IotivityClient::foundDeviceCallback, this,
              std::placeholders::_1, value);

  OCStackResult result = OCPlatform::getDeviceInfo(hostUri,
                         deviceDiscoveryRequest,
                         CT_ADAPTER_IP, deviceInfoHandler);
  if (OC_STACK_OK != result) {
    ERROR_MSG("OCPlatform::getDeviceInfo was unsuccessful\n");
    m_device->postError(async_call_id);
    return;
  }

  if (waitsec >= 0) {
    std::thread exec(std::function<void(const picojson::value &value)>(
                       std::bind(&IotivityClient::findDeviceTimerCallback,
                                 this, std::placeholders::_1)),
                     value);
    exec.detach();
  }
}

void IotivityClient::handleFindResources(const picojson::value &value) {
  DEBUG_MSG("handleFindResources: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  picojson::value param = value.get("OicDiscoveryOptions");

  // all properties are null by default, meaning “find all”
  // if resourceId is specified in full form, a direct retrieve is made
  // if resourceType is specified, a retrieve on /oic/res is made
  // if resourceId is null, and deviceId not,
  // then only resources from that device are returned
  std::string deviceId = "";
  std::string resourceId = "";
  std::string resourceType = "";
  int waitsec = GetWait(value);

  if (param.contains("deviceId")) {
    deviceId = param.get("deviceId").to_str();
  }

  if (param.contains("resourceId")) {
    resourceId = param.get("resourceId").to_str();
  }

  if (param.contains("resourceType")) {
    resourceType = param.get("resourceType").to_str();
  }

  DEBUG_MSG(
    "handleFindResources: device = %s\n"
    "\tresource = %s\n"
    "\tresourceType = %s\n"
    "\ttimeout = %d\n",
    deviceId.c_str(), resourceId.c_str(), resourceType.c_str(), waitsec);

  m_foundresourcemap.clear();

  std::string hostUri = "";
  string discoveryUri = OC_RSRVD_WELL_KNOWN_URI;
  string requestUri = discoveryUri;

  if ((deviceId == "") && (resourceId == "") && (resourceType == "")) {
    // Find all
  } else {
    if (resourceType != "") {
      requestUri = discoveryUri + "?rt=" + resourceType;
    }

    if (resourceId != "") {
      IotivityResourceClient *resClient = getResourceById(resourceId);
      if (resClient == NULL) {
        ERROR_MSG("OCPlatform::findResource by resourceId was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
      } else {
        m_foundresourcemap[resourceId] = resClient;
        findPreparedRequest(value);
        return;
      }
    }
  }

  DEBUG_MSG(
    "process: hostUri=%s, resId=%s, resType=%s, "
    "deviceId=%s, timeout=%ds\n",
    hostUri.c_str(), resourceId.c_str(), resourceType.c_str(),
    deviceId.c_str(), waitsec);

  FindCallback resourceHandler =
    std::bind(&IotivityClient::foundResourceCallback,
              this, std::placeholders::_1,
              value);

  OCStackResult result = OCPlatform::findResource(hostUri, requestUri,
                         CT_DEFAULT, resourceHandler);
  if (OC_STACK_OK != result) {
    ERROR_MSG("OCPlatform::findResource was unsuccessful\n");
    m_device->postError(async_call_id);
    return;
  }

  if (waitsec >= 0) {
    std::thread exec(std::function<void(const picojson::value &value)>(
                       std::bind(&IotivityClient::findResourceTimerCallback,
                                 this, std::placeholders::_1)),
                     value);
    exec.detach();
  }
}

void IotivityClient::handleRetrieveResource(const picojson::value &value) {
  DEBUG_MSG("handleRetrieveResource: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  std::string resId = value.get("id").to_str();
  IotivityResourceClient *resClient = getResourceById(resId);

  if (resClient != NULL) {
    OCStackResult result = resClient->retrieveResource(async_call_id);
    if (OC_STACK_OK != result) {
      m_device->postError(async_call_id);
      return;
    }
  } else {
    DEBUG_MSG("handleRetrieveResource: ERROR_MSG resource not found\n");
    m_device->postError(async_call_id);
  }
}

void IotivityClient::handleStartObserving(const picojson::value &value) {
  DEBUG_MSG("handleStartObserving: v=%s\n", value.serialize().c_str());

  OCStackResult result;
  double async_call_id = value.get("asyncCallId").get<double>();
  std::string resId = value.get("id").to_str();
  DEBUG_MSG("\tstartObserving resId = %s\n", resId.c_str());
  IotivityResourceClient *resClient = getResourceById(resId);

  if (resClient != NULL) {
    result = resClient->startObserving(async_call_id);
    if (OC_STACK_OK != result) {
      DEBUG_MSG("\tstartObserving ERROR\n");
      m_device->postError(async_call_id);
      return;
    }
  } else {
    DEBUG_MSG("\tresClient NULL ERROR\n");
    m_device->postError(async_call_id);
    return;
  }
}

void IotivityClient::handleUpdateResource(const picojson::value &value) {
  DEBUG_MSG("handleUpdateResource: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  bool doPost = value.get("doPost").get<bool>();
  picojson::value param = value.get("OicResource");
  std::string resId = param.get("id").to_str();
  IotivityResourceClient *resClient = getResourceById(resId);

  if (resClient != NULL) {
    IotivityResourceInit oicResourceInit(param);

    OCStackResult result;

    result =
      resClient->updateResource(oicResourceInit.m_resourceRep,
                                async_call_id,
                                doPost);

    if (OC_STACK_OK != result) {
      m_device->postError(async_call_id);
      return;
    }
  } else {
    m_device->postError(async_call_id);
  }
}
