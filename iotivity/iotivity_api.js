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

var _debug_enabled = 1;  // toggle this to show/hide debug logs

var g_iotivity_device = null;

///////////////////////////////////////////////////////////////////////////////
// Logging helpers
///////////////////////////////////////////////////////////////////////////////
function DBG(msg) {
  if (!_debug_enabled) return;
  console.log('[Iotivity-D]:' + msg);
}

function ERR(msg) {
  console.error('[Iotivity-E]:' + msg);
}

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

var g_next_async_call_id = 0;
var g_async_calls = {};

function AsyncCall(resolve, reject) {
  this.resolve = resolve;
  this.reject = reject;
}

function createPromise(msg) {
  var promise = new Promise(function(resolve, reject) {
    g_async_calls[g_next_async_call_id] = new AsyncCall(resolve, reject);
  });
  msg.asyncCallId = g_next_async_call_id;
  extension.postMessage(JSON.stringify(msg));
  ++g_next_async_call_id;

  return promise;
}

function _addConstProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(
      obj, propertyKey,
      {configurable: true, writable: false, value: propertyValue});
}

function _addProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: true,
    writable: true,
    enumerable: true,
    value: propertyValue
  });
}

///////////////////////////////////////////////////////////////////////////////
// EventTarget
///////////////////////////////////////////////////////////////////////////////
function EventTarget(event_types) {
  var _event_listeners = {};
  var _event_handlers = {};
  var self = this;

  if (event_types != null) {
    // initialize event listeners
    event_types.forEach(function(type) {
      _event_listeners[type] = [];
      // Setup a Event Handler
      _addProperty(self, 'on' + type, null);
    });
  }

  this._event_listeners = _event_listeners;
}

EventTarget.prototype.isValidEventType = function(type) {
  return type in this._event_listeners;
};

EventTarget.prototype.addEventListener = function(type, listener) {
  if (this.isValidEventType(type) && listener != null &&
      typeof listener === 'function' &&
      this._event_listeners[type].indexOf(listener) == -1) {
    this._event_listeners[type].push(listener);
    return;
  }
  ERR('Invalid event type \'' + type +
      '\', ignoring. Avaliable event types : ' + this._event_listeners);
};

EventTarget.prototype.removeEventListener = function(type, listener) {
  if (!type || !listener) return;

  var index = this._event_listeners[type].indexOf(listener);
  if (index == -1) return;

  this._event_listeners[type].splice(index, 1);
};

EventTarget.prototype.dispatchEvent = function(ev) {
  var handled = true;
  var listeners = null;
  if (typeof ev === 'object' && this.isValidEventType(ev.type) &&
      (listeners = this._event_listeners[ev.type]) != null) {
    listeners.forEach(function(listener) {
      if (!listener(ev) && handled) handled = false;
    });
  }
  return handled;
};

///////////////////////////////////////////////////////////////////////////////
// OicDevice
///////////////////////////////////////////////////////////////////////////////
function OicDevice(settings) {
  DBG('new OicDevice: settings=' + JSON.stringify(settings));

  if (settings)
    _addConstProperty(this, 'settings', new OicDeviceSettings(settings));
  else {
    // default settings
    _addConstProperty(this, 'settings', new OicDeviceSettings(null));
  }
  _addConstProperty(this, 'client', new OicClient());
  _addConstProperty(this, 'server', new OicServer());

  g_iotivity_device = this;
}

// partial dictionary is ok
// equivalent to ‘onboard+configure’ in Core spec
// maps to IoTivity Configure (C++ API), configure (Java API), OCInit (C API)
OicDevice.prototype.configure = function(settings) {
  DBG('OicDevice.configure=' + JSON.stringify(settings));
  if (settings)
    _addConstProperty(this, 'settings', new OicDeviceSettings(settings));

  var msg = {
    'cmd': 'configure',
    'settings': {
      'url': this.settings.url,
      'role': this.settings.role,
      'connectionMode': this.settings.connectionMode,
      'info': {
        'uuid': this.settings.info.uuid,
        'name': this.settings.info.name,
        'dataModels': this.settings.info.dataModels,
        'coreSpecVersion': this.settings.info.coreSpecVersion,
        'osVersion': this.settings.info.osVersion,
        'model': this.settings.info.model,
        'manufacturerName': this.settings.info.manufacturerName,
        'manufacturerUrl': this.settings.info.manufacturerUrl,
        'manufacturerDate': this.settings.info.manufacturerDate,
        'platformVersion': this.settings.info.platformVersion,
        'firmwareVersion': this.settings.info.firmwareVersion,
        'supportUrl': this.settings.info.supportUrl
      }
    }
  };

  return createPromise(msg);
};

// return to factory configuration and reboot
OicDevice.prototype.factoryReset = function() {
  var msg = {
    'cmd': 'factoryReset'
  };
  return createPromise(msg);
};

// keep configuration and reboot
OicDevice.prototype.reboot = function() {
  var msg = {
    'cmd': 'reboot'
  };
  return createPromise(msg);
};

iotivity.OicDevice = OicDevice;

///////////////////////////////////////////////////////////////////////////////
// OicDeviceSettings
///////////////////////////////////////////////////////////////////////////////

function OicDeviceSettings(obj) {
  DBG('new OicDeviceSettings: obj=' + JSON.stringify(obj));
  if (obj) {
    _addConstProperty(this, 'url', obj.url);
    _addConstProperty(this, 'info', new OicDeviceInfo(obj.info));
    _addConstProperty(this, 'role', obj.role);
    _addConstProperty(this, 'connectionMode', obj.connectionMode);
  } else {
    _addConstProperty(this, 'url', '0.0.0.0:0');
    _addConstProperty(this, 'info', new OicDeviceInfo(null));
    _addConstProperty(this, 'role', 'intermediate');
    _addConstProperty(this, 'connectionMode', 'acked');
  }
}

iotivity.OicDeviceSettings = OicDeviceSettings;

///////////////////////////////////////////////////////////////////////////////
// OicDeviceInfo
///////////////////////////////////////////////////////////////////////////////

function OicDeviceInfo(obj) {
  DBG('new OicDeviceInfo: obj=' + JSON.stringify(obj));
  if (obj) {
    _addConstProperty(this, 'uuid', obj.uuid || 'default');
    _addConstProperty(this, 'name', obj.name || 'default');
    _addConstProperty(this, 'dataModels', obj.dataModels || 'default');
    _addConstProperty(this, 'coreSpecVersion',
                      obj.coreSpecVersion || 'default');
    _addConstProperty(this, 'osVersion', obj.osVersion || 'default');
    _addConstProperty(this, 'model', obj.model || 'default');
    _addConstProperty(this, 'hardwareVersion', obj.hardwareVersion || 'default');
    _addConstProperty(this, 'manufacturerName',
                      obj.manufacturerName || 'default');
    _addConstProperty(this, 'manufacturerUrl',
                      obj.manufacturerUrl || 'default');
    _addConstProperty(this, 'manufacturerDate',
                      obj.manufacturerDate || 'default');
    _addConstProperty(this, 'platformVersion',
                      obj.platformVersion || 'default');
    _addConstProperty(this, 'firmwareVersion',
                      obj.firmwareVersion || 'default');
    _addConstProperty(this, 'supportUrl', obj.supportUrl || 'default');
  }
}

iotivity.OicDeviceInfo = OicDeviceInfo;


///////////////////////////////////////////////////////////////////////////////
// OicClient
///////////////////////////////////////////////////////////////////////////////

function OicClient(obj) {
  this.onresourcechange = null;
}

// client API: discovery
OicClient.prototype.findResources = function(options) {
  var msg = {
    'cmd': 'findResources',
    'OicDiscoveryOptions': options
  };
  return createPromise(msg);
};

OicClient.prototype.findDevices = function(options) {
  var msg = {
    'cmd': 'findDevices',
    'OicDiscoveryOptions': options
  };
  return createPromise(msg);
};

// client API: CRUDN
OicClient.prototype.createResource = function(resourceinit) {
  var resourceId = 0;  // todo(aphao) Missing id from spec ??
  var msg = {
    'cmd': 'createResource',
    'id': resourceId,
    'OicResourceInit': resourceinit
  };
  return createPromise(msg);
};

OicClient.prototype.retrieveResource = function(resourceId) {
  var msg = {
    'cmd': 'retrieveResource',
    'id': resourceId
  };
  return createPromise(msg);
};

OicClient.prototype.updateResource = function(resource) {
  return OicClient.prototype.updateResource(resource, false);
};

OicClient.prototype.updateResource = function(resource, doPost) {
  var msg = {
    'cmd': 'updateResource',
    'OicResource': {
      'id': resource.id,
      'url': resource.url,
      'deviceId': resource.deviceId,
      'connectionMode': resource.connectionMode,
      'resourceTypes': resource.resourceTypes,
      'interfaces': resource.interfaces,
      'discoverable': resource.discoverable,
      'observable': resource.observable,
      'parent': resource.parent || null,
      'children': resource.children || null,
      'properties': resource.properties
    },
    'doPost': doPost
  };
  return createPromise(msg);
};

OicClient.prototype.deleteResource = function(resourceId) {
  var msg = {
    'cmd': 'deleteResource',
    'id': resourceId
  };
  return createPromise(msg);
};

OicClient.prototype.startObserving = function(resourceId) {
  var msg = {
    'cmd': 'startObserving',
    'id': resourceId
  };

  return createPromise(msg);
};

OicClient.prototype.cancelObserving = function(resourceId) {
  var msg = {
    'cmd': 'cancelObserving',
    'id': resourceId
  };
  return createPromise(msg);
};

iotivity.OicClient = OicClient;

///////////////////////////////////////////////////////////////////////////////
// OicServer
///////////////////////////////////////////////////////////////////////////////

function OicServer(obj) {
  this.onrequest = null;
}

// register/unregister locally constructed resource objects with OIC
// gets an id
OicServer.prototype.registerResource = function(init) {
  var msg = {
    'cmd': 'registerResource',
    'OicResourceInit': init
  };
  return createPromise(msg);
};

// purge resource, then notify all
OicServer.prototype.unregisterResource = function(resourceId) {
  var msg = {
    'cmd': 'unregisterResource',
    'resourceId': resourceId
  };
  return createPromise(msg);
};

// enable/disable presence (discovery, state changes) for this device and its
// resources
OicServer.prototype.enablePresence = function() {
  var msg = {
    'cmd': 'enablePresence'
  };
  return createPromise(msg);
};

OicServer.prototype.disablePresence = function() {
  var msg = {
    'cmd': 'registerResource'
  };
  return createPromise(msg);
};

OicServer.prototype.notify = function(resourceId, method,
                                      updatedPropertyNames) {
  var msg = {
    'cmd': 'notify',
    'resourceId': resourceId,
    'method': method,
    'updatedPropertyNames': updatedPropertyNames
  };
  return createPromise(msg);
};

iotivity.OicServer = OicServer;

///////////////////////////////////////////////////////////////////////////////
// OicRequestEvent
///////////////////////////////////////////////////////////////////////////////
function OicRequestEvent(obj) {
  DBG('new OicRequestEvent: obj=' + JSON.stringify(obj));

  _addConstProperty(this, 'type', obj.type);
  _addConstProperty(this, 'requestId', obj.requestId);
  _addConstProperty(this, 'source', obj.source);
  _addConstProperty(this, 'target', obj.target);
  _addConstProperty(this, 'properties', obj.properties);
  _addConstProperty(this, 'updatedPropertyNames', obj.updatedPropertyNames);
  _addConstProperty(this, 'headerOptions', obj.headerOptions);
  _addConstProperty(this, 'queryOptions', obj.queryOptions);
}

// for create, observe, retrieve
// reuses request info (type, requestId, source, target) to construct response,
// sends back “ok”, plus the resource object if applicable
OicRequestEvent.prototype.sendResponse = function(resource) {
  var msg = {
    'cmd': 'sendResponse',
    'resource': resource,
    'OicRequestEvent': {
      'requestId': this.requestId,
      'type': this.type,
      'target': this.target,
      'source': this.source,
      'properties': this.properties,
      'updatedPropertyNames': this.updatedPropertyNames,
      'headerOptions': this.headerOptions,
      'queryOptions': this.queryOptions
    }
  };

  DBG('sendResponse: msg=' + JSON.stringify(msg));

  return createPromise(msg);
};

// reuses request info (type, requestId, source, target) to construct response,
// sends an Error, where error.message maps to OIC errors, see later
OicRequestEvent.prototype.sendError = function(error) {
  var msg = {
    'cmd': 'sendError',
    'error': error,
    'OicRequestEvent': {
      'requestId': this.requestId,
      'type': this.type,
      'target': this.target,
      'source': this.source,
      'properties': this.properties,
      'updatedPropertyNames': this.updatedPropertyNames,
      'headerOptions': this.headerOptions,
      'queryOptions': this.queryOptions
    }
  };

  DBG('sendResponse: msg=' + JSON.stringify(msg));

  return createPromise(msg);
};

iotivity.OicRequestEvent = OicRequestEvent;

///////////////////////////////////////////////////////////////////////////////
// OicResourceChangedEvent
///////////////////////////////////////////////////////////////////////////////

function OicResourceChangedEvent(obj) {
  DBG('new OicResourceChangedEvent: obj=' + JSON.stringify(obj));

  _addConstProperty(this, 'type', obj.type);
  _addConstProperty(this, 'resource', obj.resource);
  _addConstProperty(this, 'updatedPropertyNames', obj.updatedPropertyNames);
}

iotivity.OicResourceChangedEvent = OicResourceChangedEvent;

///////////////////////////////////////////////////////////////////////////////
// OicDiscoveryOptions, OicResourceRepresentation, OicResourceInit
///////////////////////////////////////////////////////////////////////////////

// all properties are null by default, meaning “find all
// if resourceId is specified in full form, a direct retrieve is made
// if resourceType is specified, a retrieve on /oic/res is made
// if resourceId is null, and deviceId not, then only resources from that device
// are returned
function OicDiscoveryOptions(obj) {
  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'resourceId', obj.resourceId);
  _addConstProperty(this, 'resourceType', obj.resourceType);
}

iotivity.OicDiscoveryOptions = OicDiscoveryOptions;

function OicResourceRepresentation(obj) {
  // any non-function properties that are JSON-serializable, e.g. string,
  // number, boolean, URI
}

iotivity.OicResourceRepresentation = OicResourceRepresentation;


function OicResourceInit(obj) {
  DBG('new OicResourceInit: obj=' + JSON.stringify(obj));

  _addConstProperty(this, 'url', obj.url);
  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'connectionMode', obj.connectionMode);
  _addConstProperty(this, 'resourceTypes', obj.resourceTypes);
  _addConstProperty(this, 'interfaces', obj.interfaces);
  _addConstProperty(this, 'discoverable', obj.discoverable);
  _addConstProperty(this, 'observable', obj.observable);

  // resource hierarchies, perhaps not needed in the first version
  _addConstProperty(this, 'parent', obj.parent);
  _addConstProperty(this, 'children', obj.children);

  // additional, resource type specific properties are allowed as “mixin”
  _addConstProperty(this, 'properties', obj.properties);
}

iotivity.OicResourceInit = OicResourceInit;

///////////////////////////////////////////////////////////////////////////////
// OicResource
///////////////////////////////////////////////////////////////////////////////
function OicResource(obj) {
  // id: obtained when registered, empty at construction
  DBG('new OicResource: obj=' + JSON.stringify(obj));
  // properties of OicResourceInit are exposed as readonly attributes
  _addConstProperty(this, 'url', obj.url);
  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'connectionMode', obj.connectionMode);
  _addConstProperty(this, 'resourceTypes', obj.resourceTypes);
  _addConstProperty(this, 'interfaces', obj.interfaces);
  _addConstProperty(this, 'discoverable', obj.discoverable);
  _addConstProperty(this, 'observable', obj.observable);
  // resource hierarchies, perhaps not needed in the first version
  _addConstProperty(this, 'parent', obj.parent || null);
  _addConstProperty(this, 'children', obj.children || null);
  // additional, resource type specific properties are allowed as “mixin”
  _addConstProperty(this, 'properties', obj.properties);
}

iotivity.OicResource = OicResource;

///////////////////////////////////////////////////////////////////////////////
// HeaderOption
///////////////////////////////////////////////////////////////////////////////
// see also https://fetch.spec.whatwg.org/#headers
function HeaderOption(obj) {
  _addConstProperty(this, 'name', obj.name);
  _addConstProperty(this, 'value', obj.value);
}

iotivity.HeaderOption = HeaderOption;

///////////////////////////////////////////////////////////////////////////////
// QueryOption
///////////////////////////////////////////////////////////////////////////////
function QueryOption(obj) {
  _addConstProperty(this, 'key', obj.key);
  _addConstProperty(this, 'value', obj.value);
}

iotivity.QueryOption = QueryOption;

///////////////////////////////////////////////////////////////////////////////
// Exports and main entry point for the Iotivity API
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  DBG('setMessageListener msg=' + JSON.stringify(msg));
  DBG('msg.cmd=' + msg.cmd);

  switch (msg.cmd) {
    case 'registerResourceCompleted':
      handleRegisterResourceCompleted(msg);
      break;
    case 'entityHandler':
      handleEntityHandler(msg);
      break;
    case 'findDevicesCompleted':
      handleFoundDevices(msg);
      break;
    case 'foundResourceCallback':
      handleFoundResources(msg);
      break;
    case 'onObserve':
      handleOnObserve(msg);
      break;
    case 'createResourceCompleted':
      handleCreateResourceCompleted(msg);
      break;
    case 'retrieveResourceCompleted':
    case 'startObservingCompleted':
      handleRetrieveResourceCompleted(msg);
      break;
    case 'updateResourceCompleted':
      handleUpdateResourceCompleted(msg);
      break;
    case 'deleteResourceCompleted':
      handleDeleteResourceCompleted(msg);
      break;
    case 'configureCompleted':
    case 'unregisterResourceCompleted':
    case 'enablePresenceCompleted':
    case 'disablePresenceCompleted':
    case 'sendResponseCompleted':
    case 'notifyCompleted':
    case 'cancelObservingCompleted':
      handleAsyncCallSuccess(msg);
      break;
    case 'asyncCallError':
      handleAsyncCallError(msg);
      break;
    default:
      DBG('Received unknown command');
      break;
  }
});

function handleRegisterResourceCompleted(msg) {
  DBG('handleRegisterResourceCompleted');
  DBG('msg.OicResourceInit=' + JSON.stringify(msg.OicResourceInit));
  var oicResource = new OicResource(msg.OicResourceInit);
  _addConstProperty(oicResource, 'id', msg.id);
  g_async_calls[msg.asyncCallId].resolve(oicResource);
}

function handleEntityHandler(msg) {
  DBG('handleEntityHandler msg=' + JSON.stringify(msg));

  if (g_iotivity_device && g_iotivity_device.server &&
      g_iotivity_device.server.onrequest) {
    var oicRequestEvent = new OicRequestEvent(msg.OicRequestEvent);
    DBG('handleEntityHandler oicRequestEvent=' +
        JSON.stringify(oicRequestEvent));
    g_iotivity_device.server.onrequest(oicRequestEvent);
  }
}

function handleFoundDevices(msg) {
  DBG('handleFoundDevices msg=' + JSON.stringify(msg));

  var oicDeviceList = [];
  for (var i = 0; i < msg.devicesArray.length; i++) {
    var oicDeviceObject = msg.devicesArray[i];
    var oicDeviceInfo = new OicDeviceInfo(oicDeviceObject.info);
    oicDeviceList.push(oicDeviceInfo);
  }

  if (msg.asyncCallId in g_async_calls) {
    if (oicDeviceList.length) {
      DBG('g_async_calls[].resolve');
      g_async_calls[msg.asyncCallId].resolve(oicDeviceList);
    } else {
      g_async_calls[msg.asyncCallId].reject(Error('Find devices error'));
    }
  }
}

function handleFoundResources(msg) {
  DBG('handleFoundResources msg=' + JSON.stringify(msg));

  var oicResourceList = [];
  for (var i = 0; i < msg.resourcesArray.length; i++) {
    var oicResourceObject = msg.resourcesArray[i];
    var oicResource = new OicResource(oicResourceObject.OicResourceInit);
    _addConstProperty(oicResource, 'id', oicResourceObject.id);
    oicResourceList.push(oicResource);
  }

  if (msg.asyncCallId in g_async_calls) {
    if (oicResourceList.length) {
      DBG('g_async_calls[].resolve');
      g_async_calls[msg.asyncCallId].resolve(oicResourceList);
    } else {
      g_async_calls[msg.asyncCallId].reject(Error('Find resources error'));
    }
  }
}

function handleOnObserve(msg) {
  DBG('handleOnObserve msg=' + JSON.stringify(msg));

  if (g_iotivity_device && g_iotivity_device.client &&
      g_iotivity_device.client.onresourcechange) {
    var oicResource = new OicResource(msg.OicResourceInit);
    _addConstProperty(oicResource, 'id', msg.id);

    var oicResourceChangedEvent = new OicResourceChangedEvent({
      'type': msg.type,
      'resource': oicResource,
      'updatedPropertyNames': msg.updatedPropertyNames
    });

    g_iotivity_device.client.onresourcechange(oicResourceChangedEvent);
  }
}

function handleCreateResourceCompleted(msg) {
  DBG('handleCreateResourceCompleted msg=' + JSON.stringify(msg));

  if (msg.asyncCallId in g_async_calls) {
    if (msg.eCode == 0) {
      DBG('g_async_calls[].resolve');

      var oicResource = new OicResource(msg.OicResourceInit);
      _addConstProperty(oicResource, 'id', msg.id);

      g_async_calls[msg.asyncCallId].resolve(oicResource);
    } else {
      g_async_calls[msg.asyncCallId].reject(Error('Command error'));
    }

    delete g_async_calls[msg.asyncCallId];
  }
}

function handleRetrieveResourceCompleted(msg) {
  DBG('handleRetrieveResourceCompleted msg=' + JSON.stringify(msg));

  if (msg.asyncCallId in g_async_calls) {
    if (msg.eCode == 0) {
      DBG('g_async_calls[].resolve');
      var oicResource = new OicResource(msg.OicResourceInit);
      _addConstProperty(oicResource, 'id', msg.id);
      g_async_calls[msg.asyncCallId].resolve(oicResource);
    } else {
      g_async_calls[msg.asyncCallId].reject(Error('Command error'));
    }

    delete g_async_calls[msg.asyncCallId];
  }
}

function handleUpdateResourceCompleted(msg) {
  DBG('handleUpdateResourceCompleted msg=' + JSON.stringify(msg));

  if (msg.asyncCallId in g_async_calls) {
    if (msg.eCode == 0) {
      DBG('g_async_calls[].resolve');
      g_async_calls[msg.asyncCallId].resolve();
    } else {
      g_async_calls[msg.asyncCallId].reject(Error('Command error'));
    }

    delete g_async_calls[msg.asyncCallId];
  }
}

function handleDeleteResourceCompleted(msg) {
  DBG('handleDeleteResourceCompleted msg=' + JSON.stringify(msg));

  if (msg.asyncCallId in g_async_calls) {
    if (msg.eCode == 0) {
      DBG('g_async_calls[].resolve');
      g_async_calls[msg.asyncCallIde].resolve();
    } else {
      g_async_calls[msg.asyncCallId].reject(Error('Command error'));
    }

    delete g_async_calls[msg.asyncCallId];
  }
}


function handleAsyncCallSuccess(msg) {
  if (msg.asyncCallId in g_async_calls) {
    g_async_calls[msg.asyncCallId].resolve();
    delete g_async_calls[msg.asyncCallId];
  }
}

function handleAsyncCallError(msg) {
  if (msg.asyncCallId in g_async_calls) {
    g_async_calls[msg.asyncCallId].reject(Error('Async operation failed'));
    delete g_async_calls[msg.asyncCallId];
  }
}

exports.handleMessageAsync = function(msg, callback) {
  iotivityListener = callback;
  extension.postMessage(msg);
};

exports.handleMessageSync = function(msg) {
  return extension.internal.sendSyncMessage(msg);
};
