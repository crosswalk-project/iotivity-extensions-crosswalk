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
#include "iotivity/iotivity_resource.h"
#include "common/extension.h"

IotivityResourceInit::IotivityResourceInit() {
  m_url = "";
  m_deviceId = "";
  m_connectionMode = "";
  m_discoverable = false;
  m_observable = false;
  m_isSecure = false;
}

IotivityResourceInit::IotivityResourceInit(const picojson::value& value) {
  deserialize(value);
}

IotivityResourceInit::~IotivityResourceInit() {}

void IotivityResourceInit::deserialize(const picojson::value& value) {
  OC_LOG_V(DEBUG, TAG, ">>IotivityResourceInit::deserialize\n");

  m_url = value.get("url").to_str();
  m_deviceId = value.get("deviceId").to_str();
  m_connectionMode = value.get("connectionMode").to_str();
  m_discoverable = value.get("discoverable").get<bool>();
  m_observable = value.get("observable").get<bool>();
  m_isSecure = false;
  m_resourceTypeName = "";
  m_resourceInterface = "";
  m_resourceProperty = 0;

  picojson::array resourceTypes =
    value.get("resourceTypes").get<picojson::array>();

  for (picojson::array::iterator iter = resourceTypes.begin();
       iter != resourceTypes.end(); ++iter) {
    OC_LOG_V(DEBUG, TAG, "array resourceTypes value=%s\n",
      (*iter).get<string>().c_str());
    if (m_resourceTypeName == "") {
      m_resourceTypeName = (*iter).get<string>();
    }

    m_resourceTypeNameArray.push_back((*iter).get<string>());
  }

  picojson::array interfaces = value.get("interfaces").get<picojson::array>();

  for (picojson::array::iterator iter = interfaces.begin();
       iter != interfaces.end(); ++iter) {
    OC_LOG_V(DEBUG, TAG, "array interfaces value=%s\n",
      (*iter).get<string>().c_str());
    if (m_resourceInterface == "") {
      m_resourceInterface = (*iter).get<string>();
    }
    m_resourceInterfaceArray.push_back((*iter).get<string>());
  }

  if (m_resourceInterface == "") { m_resourceInterface = DEFAULT_INTERFACE; }

  if (m_discoverable) { m_resourceProperty |= OC_DISCOVERABLE; }

  if (m_observable) { m_resourceProperty |= OC_OBSERVABLE; }

  if (m_isSecure) { m_resourceProperty |= OC_SECURE; }

  OC_LOG_V(DEBUG, TAG, "discoverable=%d, observable=%d, isSecure=%d\n",
    m_discoverable, m_observable, m_isSecure);
  OC_LOG_V(DEBUG, TAG, "SVR: uri=%s, type=%s, itf=%s, prop=%d\n",
    m_url.c_str(), m_resourceTypeName.c_str(), m_resourceInterface.c_str(),
            m_resourceProperty);

  picojson::value properties = value.get("properties");
  picojson::object& propertiesobject = properties.get<picojson::object>();
  OC_LOG_V(DEBUG, TAG, "properties: size=%d\n", propertiesobject.size());

  m_resourceRep.setUri(m_url);
  PicojsonPropsToOCRep(m_resourceRep, propertiesobject);
  OC_LOG_V(DEBUG, TAG, "<<IotivityResourceInit::deserialize\n");
}

void IotivityResourceInit::serialize(picojson::object& object) {
  object["url"] = picojson::value(m_url);
  object["deviceId"] = picojson::value(m_deviceId);
  object["connectionMode"] = picojson::value(m_connectionMode);
  object["discoverable"] = picojson::value(m_discoverable);
  object["observable"] = picojson::value(m_observable);
  picojson::array resourceTypes;

  CopyInto(m_resourceTypeNameArray, resourceTypes);

  object["resourceTypes"] = picojson::value(resourceTypes);
  picojson::array interfaces;

  CopyInto(m_resourceInterfaceArray, interfaces);

  object["interfaces"] = picojson::value(interfaces);
  picojson::object properties;
  TranslateOCRepresentationToPicojson(m_resourceRep, properties);
  object["properties"] = picojson::value(properties);
}

IotivityResourceServer::IotivityResourceServer(
  IotivityDevice* device, IotivityResourceInit* oicResource)
  : m_device(device) {
  m_oicResourceInit = oicResource;
  m_resourceHandle = 0;
}

IotivityResourceServer::~IotivityResourceServer() {
  if (m_resourceHandle != 0) {
    OCPlatform::unregisterResource(m_resourceHandle);
    m_resourceHandle = 0;
  }

  if (m_oicResourceInit) {
    delete m_oicResourceInit;
    m_oicResourceInit = NULL;
  }
}

OCRepresentation IotivityResourceServer::getRepresentation() {
  return m_oicResourceInit->m_resourceRep;
}

ObservationIds& IotivityResourceServer::getObserversList() {
  return m_interestedObservers;
}

OCEntityHandlerResult IotivityResourceServer::entityHandlerCallback(
  std::shared_ptr<OCResourceRequest> request) {
  OC_LOG_V(DEBUG, TAG, "\n\n[Remote Client==>] entityHandlerCallback:\n");

  OCEntityHandlerResult ehResult = OC_EH_ERROR;
  picojson::value::object object;

  if (request) {
    ehResult = OC_EH_OK;
    IotivityRequestEvent iotivityRequestEvent;
    iotivityRequestEvent.deserialize(request);
    iotivityRequestEvent.m_resourceRepTarget = m_oicResourceInit->m_resourceRep;
    int requestFlag = request->getRequestHandlerFlag();

    if (requestFlag & RequestHandlerFlag::ObserverFlag) {
      OC_LOG_V(DEBUG, TAG, "postEntityHandler:ObserverFlag\n");

      iotivityRequestEvent.m_type = "observe";
      ObservationInfo observationInfo = request->getObservationInfo();

      if (ObserveAction::ObserveRegister == observationInfo.action) {
        OC_LOG_V(DEBUG, TAG, "postEntityHandler:ObserveRegister\n");
        m_interestedObservers.push_back(observationInfo.obsId);
      } else if (ObserveAction::ObserveUnregister == observationInfo.action) {
        OC_LOG_V(DEBUG, TAG, "postEntityHandler:ObserveUnregister\n");
        m_interestedObservers.erase(
          std::remove(m_interestedObservers.begin(),
                      m_interestedObservers.end(), observationInfo.obsId),
          m_interestedObservers.end());
      }
    }

    if (iotivityRequestEvent.m_type == "update") {
      UpdateOcRepresentation(iotivityRequestEvent.m_resourceRep,
                             m_oicResourceInit->m_resourceRep,
                             iotivityRequestEvent.m_updatedPropertyNames);
    }

    object["cmd"] = picojson::value("entityHandler");
    picojson::object OicRequestEvent;
    iotivityRequestEvent.serialize(OicRequestEvent);
    object["OicRequestEvent"] = picojson::value(OicRequestEvent);
    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
  } else {
    OC_LOG_V(ERROR, TAG, "entityHandlerCallback: Request invalid");
  }

  return ehResult;
}

void IotivityResourceServer::serialize(picojson::object& object) {
  object["id"] = picojson::value(m_idfull);
  picojson::object properties;
  m_oicResourceInit->serialize(properties);
  object["OicResourceInit"] = picojson::value(properties);
}

OCStackResult IotivityResourceServer::registerResource() {
  OCStackResult result = OC_STACK_ERROR;
  EntityHandler resourceCallback =
    std::bind(&IotivityResourceServer::entityHandlerCallback, this,
              std::placeholders::_1);

  result = OCPlatform::registerResource(
             m_resourceHandle, m_oicResourceInit->m_url,
             m_oicResourceInit->m_resourceTypeName,
             m_oicResourceInit->m_resourceInterface, resourceCallback,
             m_oicResourceInit->m_resourceProperty);

  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "registerResource was unsuccessful\n");
    return result;
  }

  // TODO(aphao) should retrieve host IP + uri instead of int:
  // Missing C/C++ API to retrieve server resource's host url
  m_idfull = std::to_string(getResourceHandleToInt());

  unsigned int size = m_oicResourceInit->m_resourceTypeNameArray.size();

  for (unsigned int i = 1; i < size; i++) {
    std::string resourceTypeName =
      m_oicResourceInit->m_resourceTypeNameArray[i];

    OC_LOG_V(DEBUG, TAG, "bindTypeToResource=%s\n", resourceTypeName.c_str());

    result =
      OCPlatform::bindTypeToResource(m_resourceHandle, resourceTypeName);

    if (OC_STACK_OK != result) {
      OC_LOG_V(ERROR, TAG,
        "bindTypeToResource TypeName to Resource unsuccessful\n");
      return result;
    }
  }

  int iSize = m_oicResourceInit->m_resourceInterfaceArray.size();
  for (int i = 1; i < iSize; i++) {
    std::string resourceInterface =
      m_oicResourceInit->m_resourceInterfaceArray[i];

    OC_LOG_V(DEBUG, TAG, "bindInterfaceToResource=%s\n",
      resourceInterface.c_str());

    result = OCPlatform::bindInterfaceToResource(m_resourceHandle,
             resourceInterface);

    if (OC_STACK_OK != result) {
      OC_LOG_V(ERROR, TAG, "Binding InterfaceName to Resource unsuccessful\n");
      return result;
    }
  }

  return result;
}

int IotivityResourceServer::getResourceHandleToInt() {
  int* p = reinterpret_cast<int*>(m_resourceHandle);
  int pint = reinterpret_cast<int>(p);
  return pint;
}

std::string IotivityResourceServer::getResourceId() { return m_idfull; }

IotivityResourceClient::IotivityResourceClient(IotivityDevice* device)
  : m_device(device) {
  m_ocResourcePtr = NULL;
  m_oicResourceInit = new IotivityResourceInit();
}

IotivityResourceClient::~IotivityResourceClient() {
  if (m_oicResourceInit) {
    delete m_oicResourceInit;
    m_oicResourceInit = NULL;
  }
}

const std::shared_ptr<OCResource> IotivityResourceClient::getSharedPtr() {
  return m_ocResourcePtr;
}

void IotivityResourceClient::setSharedPtr(
  std::shared_ptr<OCResource> sharePtr) {
  m_ocResourcePtr = sharePtr;
  int* p = reinterpret_cast<int*>(sharePtr.get());
  int pint = reinterpret_cast<int>(p);
  m_id = pint;
  m_oicResourceInit->m_url = sharePtr->uri();
  m_oicResourceInit->m_deviceId = sharePtr->sid();
  m_oicResourceInit->m_connectionMode = "default";
  // TODO(aphao) : m_discoverable missing info from shared_ptr
  m_oicResourceInit->m_discoverable = sharePtr->isObservable();
  m_oicResourceInit->m_observable = sharePtr->isObservable();
  m_oicResourceInit->m_isSecure = false;
  m_sid = sharePtr->sid();
  m_host = sharePtr->host();
  m_idfull = m_host + sharePtr->uri();

  for (auto& resourceTypes : sharePtr->getResourceTypes()) {
    m_oicResourceInit->m_resourceTypeNameArray.push_back(resourceTypes.c_str());
  }

  for (auto& resourceInterfaces : sharePtr->getResourceInterfaces()) {
    m_oicResourceInit->m_resourceInterfaceArray.push_back(
      resourceInterfaces.c_str());
  }
}

int IotivityResourceClient::getResourceHandleToInt() { return m_id; }

std::string IotivityResourceClient::getResourceId() { return m_idfull; }

void IotivityResourceClient::serialize(picojson::object& object) {
  object["id"] = picojson::value(getResourceId());

  picojson::object properties;
  m_oicResourceInit->serialize(properties);
  object["OicResourceInit"] = picojson::value(properties);
}

void IotivityResourceClient::onPut(const HeaderOptions& headerOptions,
                                   const OCRepresentation& rep, const int eCode,
                                   double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "onPut: eCode=%d, asyncCallId=%f\n", eCode, asyncCallId);

  picojson::value::object object;
  object["cmd"] = picojson::value("updateResourceCompleted");
  object["eCode"] = picojson::value(static_cast<double>(eCode));
  object["asyncCallId"] = picojson::value(static_cast<double>(asyncCallId));

  if (eCode == SUCCESS_RESPONSE) {
    PrintfOcRepresentation(rep);
    m_oicResourceInit->m_resourceRep = rep;
    serialize(object);
  } else {
    OC_LOG_V(ERROR, TAG, "onPut was unsuccessful\n");
  }

  picojson::value value(object);
  m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onGet(const HeaderOptions& headerOptions,
                                   const OCRepresentation& rep,
                                   const int eCode,
                                   double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "onGet: eCode=%d, %f\n", eCode, asyncCallId);

  picojson::value::object object;
  object["cmd"] = picojson::value("retrieveResourceCompleted");
  object["eCode"] = picojson::value(static_cast<double>(eCode));

  object["asyncCallId"] = picojson::value(static_cast<double>(asyncCallId));

  if (eCode == SUCCESS_RESPONSE) {
    PrintfOcRepresentation(rep);
    m_oicResourceInit->m_resourceRep = rep;
    serialize(object);
  } else {
    OC_LOG_V(ERROR, TAG, "onGet was unsuccessful\n");
  }

  picojson::value value(object);
  m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onPost(const HeaderOptions& headerOptions,
                                    const OCRepresentation& rep,
                                    const int eCode, double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "onPost: eCode=%d, %f\n", eCode, asyncCallId);

  picojson::value::object object;
  object["cmd"] = picojson::value("createResourceCompleted");
  object["eCode"] = picojson::value(static_cast<double>(eCode));
  object["asyncCallId"] = picojson::value(static_cast<double>(asyncCallId));

  if (eCode == SUCCESS_RESPONSE) {
    PrintfOcRepresentation(rep);
    m_oicResourceInit->m_resourceRep = rep;
    serialize(object);
  } else {
    OC_LOG_V(ERROR, TAG, "onPost was unsuccessful\n");
  }

  picojson::value value(object);
  m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onStartObserving(double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "onStartObserving: %f\n", asyncCallId);

  picojson::value::object object;
  object["cmd"] = picojson::value("startObservingCompleted");
  object["eCode"] = picojson::value(static_cast<double>(0));
  object["asyncCallId"] = picojson::value(static_cast<double>(asyncCallId));

  serialize(object);
  picojson::value value(object);
  m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onObserve(const HeaderOptions headerOptions,
                                       const OCRepresentation& rep,
                                       const int& eCode,
                                       const int& sequenceNumber,
                                       double asyncCallId) {
  OC_LOG_V(DEBUG, TAG,
    "\n\n[Remote Server==>] "
    "onObserve: sequenceNumber=%d, eCode=%d, %f\n",
    sequenceNumber, eCode, asyncCallId);
  picojson::value::object object;
  object["cmd"] = picojson::value("onObserve");
  object["eCode"] = picojson::value(static_cast<double>(eCode));
  object["asyncCallId"] = picojson::value(static_cast<double>(asyncCallId));

  if (sequenceNumber == OC_OBSERVE_REGISTER) {
    object["type"] = picojson::value("register");
  } else if (sequenceNumber == OC_OBSERVE_DEREGISTER) {
    object["type"] = picojson::value("deregister");
  } else {
    object["type"] = picojson::value("update");
  }

  if (eCode == OC_STACK_OK) {
    PrintfOcRepresentation(rep);
    m_oicResourceInit->m_resourceRep = rep;

    std::vector<std::string> updatedPropertyNames;

    for (auto& cur : rep) {
      std::string attrname = cur.attrname();
      updatedPropertyNames.push_back(attrname);
    }

    picojson::array updatedPropertyNamesArray;
    CopyInto(updatedPropertyNames, updatedPropertyNamesArray);
    object["updatedPropertyNames"] = picojson::value(updatedPropertyNamesArray);
    serialize(object);
  } else {
    OC_LOG_V(ERROR, TAG, "\n\n[Remote Server==>] onObserve: error\n");
  }

  picojson::value value(object);
  m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onDelete(const HeaderOptions& headerOptions,
                                      const int eCode, double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "onDelete: eCode=%d, %f\n", eCode, asyncCallId);

  picojson::value::object object;
  object["cmd"] = picojson::value("deleteResourceCompleted");
  object["eCode"] = picojson::value(static_cast<double>(eCode));
  object["asyncCallId"] = picojson::value(static_cast<double>(asyncCallId));

  if (eCode != SUCCESS_RESPONSE) {
    OC_LOG_V(ERROR, TAG, "onDelete was unsuccessful\n");
  }

  picojson::value value(object);
  m_device->PostMessage(value.serialize().c_str());
}

OCStackResult IotivityResourceClient::createResource(
  IotivityResourceInit& oicResourceInit, double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "createResource %f\n", asyncCallId);

  OCStackResult result = OC_STACK_ERROR;

  if (m_ocResourcePtr == NULL) { return result; }

  PostCallback attributeHandler =
    std::bind(&IotivityResourceClient::onPost, this, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3, asyncCallId);
  result = m_ocResourcePtr->post(oicResourceInit.m_resourceRep,
                                 QueryParamsMap(), attributeHandler);
  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "post/create was unsuccessful\n");
    return result;
  }

  return result;
}

OCStackResult IotivityResourceClient::retrieveResource(double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "retrieveResource %f\n", asyncCallId);

  OCStackResult result = OC_STACK_ERROR;

  if (m_ocResourcePtr == NULL) { return result; }

  GetCallback attributeHandler =
    std::bind(&IotivityResourceClient::onGet, this, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3, asyncCallId);
  result = m_ocResourcePtr->get(QueryParamsMap(), attributeHandler);
  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "get was unsuccessful\n");
    return result;
  }

  return result;
}

OCStackResult IotivityResourceClient::updateResource(
  OCRepresentation& representation, double asyncCallId) {
  return IotivityResourceClient::updateResource(representation, asyncCallId,
         false);
}

OCStackResult IotivityResourceClient::updateResource(
  OCRepresentation& representation, double asyncCallId, bool doPost) {
  OC_LOG_V(DEBUG, TAG, "updateResource %f\n", asyncCallId);

  OCStackResult result = OC_STACK_ERROR;

  if (m_ocResourcePtr == NULL) { return result; }

  PrintfOcRepresentation(representation);

  if (doPost) {
    PostCallback attributeHandler =
      std::bind(&IotivityResourceClient::onPost, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, asyncCallId);
    result =
      m_ocResourcePtr->post(representation, QueryParamsMap(), attributeHandler);
    if (OC_STACK_OK != result) {
      OC_LOG_V(ERROR, TAG, "update was unsuccessful\n");
      return result;
    }
  } else {
    PutCallback attributeHandler =
      std::bind(&IotivityResourceClient::onPut, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, asyncCallId);
    result =
      m_ocResourcePtr->put(representation, QueryParamsMap(), attributeHandler);
    if (OC_STACK_OK != result) {
      OC_LOG_V(ERROR, TAG, "update was unsuccessful\n");
      return result;
    }
  }

  return result;
}

OCStackResult IotivityResourceClient::deleteResource(double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "deleteResource %f\n", asyncCallId);

  OCStackResult result = OC_STACK_ERROR;

  if (m_ocResourcePtr == NULL) { return result; }

  DeleteCallback deleteHandler =
    std::bind(&IotivityResourceClient::onDelete, this, std::placeholders::_1,
              std::placeholders::_2, asyncCallId);
  result = m_ocResourcePtr->deleteResource(deleteHandler);

  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "delete was unsuccessful\n");
    return result;
  }

  return result;
}

OCStackResult IotivityResourceClient::startObserving(double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "startObserving %f\n", asyncCallId);

  OCStackResult result = OC_STACK_ERROR;

  if (m_ocResourcePtr == NULL) { return result; }

  ObserveCallback observeHandler =
    std::bind(&IotivityResourceClient::onObserve, this, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3,
              std::placeholders::_4, asyncCallId);

  result = m_ocResourcePtr->observe(ObserveType::Observe, QueryParamsMap(),
                                    observeHandler);
  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "observe was unsuccessful\n");
    return result;
  }

  onStartObserving(asyncCallId);

  return result;
}

OCStackResult IotivityResourceClient::cancelObserving(double asyncCallId) {
  OC_LOG_V(DEBUG, TAG, "cancelObserving %f\n", asyncCallId);
  OCStackResult result = OC_STACK_ERROR;

  if (m_ocResourcePtr == NULL) { return result; }

  result = m_ocResourcePtr->cancelObserve();
  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "cancelObserve was unsuccessful\n");
    return result;
  }

  return result;
}

IotivityRequestEvent::IotivityRequestEvent() {}

IotivityRequestEvent::~IotivityRequestEvent() {}

void IotivityRequestEvent::deserialize(
  std::shared_ptr<OCResourceRequest> request) {
  std::string requestType = request->getRequestType();
  int requestFlag = request->getRequestHandlerFlag();
  int requestHandle = (int)request->getRequestHandle();  // NOLINT

  OC_LOG_V(DEBUG, TAG, "Deserialize: requestType=%s\n", requestType.c_str());
  OC_LOG_V(DEBUG, TAG, "Deserialize: requestFlag=0x%x\n", requestFlag);
  OC_LOG_V(DEBUG, TAG, "Deserialize: requestHandle=0x%x\n", requestHandle);

  if (requestFlag & RequestHandlerFlag::RequestFlag) {
    if (requestType == "GET") {
      m_type = "retrieve";
    } else if (requestType == "PUT") {
      m_type = "update";
    } else if (requestType == "POST") {
      // m_type = "create";
      // TODO(aphao) : or update
      m_type = "update";
    } else if (requestType == "DELETE") {
      m_type = "delete";
    }

    m_resourceRep = request->getResourceRepresentation();
    PrintfOcRepresentation(m_resourceRep);

    for (auto& cur : m_resourceRep) {
      std::string attrname = cur.attrname();
      m_updatedPropertyNames.push_back(attrname);
    }
  }

  m_queries = request->getQueryParameters();

  if (!m_queries.empty()) {
    for (auto it : m_queries) {
      OC_LOG_V(DEBUG, TAG, "Queries: key=%s, value=%s\n", it.first.c_str(),
                it.second.c_str());
    }
  }

  m_headerOptions = request->getHeaderOptions();

  if (!m_headerOptions.empty()) {
    for (auto it = m_headerOptions.begin(); it != m_headerOptions.end(); ++it) {
      OC_LOG_V(DEBUG, TAG, "HeaderOptions: ID=%d, value=%s\n",
        it->getOptionID(), it->getOptionData().c_str());
    }
  }

  m_requestId = requestHandle;
  m_source = std::to_string(requestHandle);
  m_target = std::to_string((int)(request->getResourceHandle()));  // NOLINT
}

void IotivityRequestEvent::deserialize(const picojson::value& value) {
  m_requestId = static_cast<int>(value.get("requestId").get<double>());
  m_type = value.get("type").to_str();
  m_source = value.get("source").to_str();
  m_target = value.get("target").to_str();
  picojson::value properties = value.get("properties");
  picojson::object& propertiesobject = properties.get<picojson::object>();

  OC_LOG_V(DEBUG, TAG, "IotivityRequestEvent::deserialize from JSON\n");
  OC_LOG_V(DEBUG, TAG, "properties: size=%d\n", propertiesobject.size());

  m_resourceRep.setUri(properties.get("uri").to_str());

  PicojsonPropsToOCRep(m_resourceRep, propertiesobject);
}

void IotivityRequestEvent::serialize(picojson::object& object) {
  object["type"] = picojson::value(m_type);
  object["requestId"] = picojson::value(static_cast<double>(m_requestId));
  object["source"] = picojson::value(m_source);
  object["target"] = picojson::value(m_target);

  OC_LOG_V(DEBUG, TAG, "IotivityRequestEvent::serialize to JSON\n");

  if ((m_type == "create") || (m_type == "update")) {
    picojson::object properties;
    TranslateOCRepresentationToPicojson(m_resourceRep, properties);
    object["properties"] = picojson::value(properties);

    PrintfOcRepresentation(m_resourceRep);
  }

  if ((m_type == "retrieve") || (m_type == "observe")) {
    picojson::object properties;
    TranslateOCRepresentationToPicojson(m_resourceRepTarget, properties);
    object["properties"] = picojson::value(properties);

    PrintfOcRepresentation(m_resourceRepTarget);
  }

  if (m_type == "update") {
    picojson::array updatedPropertyNamesArray;

    CopyInto(m_updatedPropertyNames, updatedPropertyNamesArray);
    object["updatedPropertyNames"] = picojson::value(updatedPropertyNamesArray);
  }

  picojson::array queryOptionsArray;
  if (!m_queries.empty()) {
    for (auto it : m_queries) {
      picojson::object object;
      OC_LOG_V(DEBUG, TAG, "Queries: key=%s, value=%s\n", it.first.c_str(),
                it.second.c_str());
      object[it.first.c_str()] = picojson::value(it.second);
      queryOptionsArray.push_back(picojson::value(object));
    }
  }

  picojson::array headerOptionsArray;

  if (!m_headerOptions.empty()) {
    picojson::object object;

    for (auto it = m_headerOptions.begin(); it != m_headerOptions.end(); ++it) {
      picojson::object object;
      OC_LOG_V(DEBUG, TAG, "HeaderOptions: ID=%d, value=%s\n",
        it->getOptionID(), it->getOptionData().c_str());
      object[std::to_string(it->getOptionID()).c_str()] =
        picojson::value(it->getOptionData());
      headerOptionsArray.push_back(picojson::value(object));
    }
  }
}

OCStackResult IotivityRequestEvent::sendResponse() {
  OC_LOG_V(DEBUG, TAG, "IotivityRequestEvent::sendResponse: type=%s\n",
    m_type.c_str());

  OCStackResult result = OC_STACK_ERROR;
  auto pResponse = std::make_shared<OC::OCResourceResponse>();
  pResponse->setRequestHandle(reinterpret_cast<void*>(m_requestId));

  int targetId = atoi(m_target.c_str());
  pResponse->setResourceHandle(reinterpret_cast<void*>(targetId));

  if (m_type == "retrieve") {
    // Send targetId representation
    pResponse->setResourceRepresentation(m_resourceRep);
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);
  } else if (m_type == "update") {
    pResponse->setResourceRepresentation(m_resourceRep);
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);
  } else if (m_type == "create") {
    // TODO(aphao) POST (with special flags ??)
    // OCRepresentation rep = pRequest->getResourceRepresentation();
    // OCRepresentation rep_post = post(rep);
    pResponse->setResourceRepresentation(m_resourceRep);
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);
  } else if (m_type == "observe") {
    pResponse->setResourceRepresentation(m_resourceRep);
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);
  }

  pResponse->setHeaderOptions(m_headerOptions);
  result = OCPlatform::sendResponse(pResponse);

  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "OCPlatform::sendResponse was unsuccessful\n");
  }

  return result;
}

OCStackResult IotivityRequestEvent::sendError() {
  OCStackResult result = OC_STACK_ERROR;
  auto pResponse = std::make_shared<OC::OCResourceResponse>();
  pResponse->setRequestHandle(reinterpret_cast<void*>(m_requestId));

  int targetId = atoi(m_target.c_str());
  pResponse->setResourceHandle(reinterpret_cast<void*>(targetId));

  // value.get("error").to_str()
  pResponse->setErrorCode(200);
  pResponse->setResponseResult(OC_EH_ERROR);
  result = OCPlatform::sendResponse(pResponse);

  if (OC_STACK_OK != result) {
    OC_LOG_V(ERROR, TAG, "OCPlatform::sendError was unsuccessful\n");
  }

  return result;
}
