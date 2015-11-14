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
#include "iotivity/iotivity_tools.h"
#include <sstream>

char* pDebugEnv = NULL;

void PrintfOcResource(const OCResource& oCResource) {
  DEBUG_MSG("PrintfOcResource\n");
  DEBUG_MSG("\tRes[sId] = %s\n", oCResource.sid().c_str());
  DEBUG_MSG("\tRes[Uri] = %s\n", oCResource.uri().c_str());
  DEBUG_MSG("\tRes[Host] = %s\n", oCResource.host().c_str());
  DEBUG_MSG("\tRes[Resource types]:\n");

  for (const auto& resourceTypes : oCResource.getResourceTypes()) {
    DEBUG_MSG("\t\t%s\n", resourceTypes.c_str());
  }

  DEBUG_MSG("Res[Resource interfaces] \n");

  for (const auto& resourceInterfaces : oCResource.getResourceInterfaces()) {
    DEBUG_MSG("\t\t%s\n", resourceInterfaces.c_str());
  }
}

void PrintfOcRepresentation(const OCRepresentation& oCRepr) {
  DEBUG_MSG(">>PrintfOcRepresentation:\n");

  std::string uri = oCRepr.getUri();
  DEBUG_MSG("\turi=%s\n", uri.c_str());

  // IOT-828
  //DEBUG_MSG("\thost=%s\n", oCRepr.getHost().c_str());

  DEBUG_MSG("\ttypes=%s\n", uri.c_str());
  for (const auto& resourceTypes : oCRepr.getResourceTypes()) {
    DEBUG_MSG("\t\t%s\n", resourceTypes.c_str());
  }

  DEBUG_MSG("\tinterfaces=%s\n", uri.c_str());
  for (const auto& resourceInterfaces : oCRepr.getResourceInterfaces()) {
    DEBUG_MSG("\t\t%s\n", resourceInterfaces.c_str());
  }

  for (const auto& cur : oCRepr) {
    std::string attrname = cur.attrname();
    DEBUG_MSG("\tattr  key=%s\n", attrname.c_str());

    if (AttributeType::String == cur.type()) {
      std::string curStr = cur.getValue<string>();
      DEBUG_MSG("\tRep[String]: key=%s, value=%s\n", attrname.c_str(),
                curStr.c_str());
    } else if (AttributeType::Integer == cur.type()) {
      DEBUG_MSG("\tRep[Integer]: key=%s, value=%d\n", attrname.c_str(),
                cur.getValue<int>());
    } else if (AttributeType::Double == cur.type()) {
      DEBUG_MSG("\tRep[Double]: key=%s, value=%f\n", attrname.c_str(),
                cur.getValue<double>());
    } else if (AttributeType::Boolean == cur.type()) {
      DEBUG_MSG("\tRep[Boolean]: key=%s, value=%d\n", attrname.c_str(),
                cur.getValue<bool>());
    } else if (AttributeType::OCRepresentation == cur.type()) {
      DEBUG_MSG("\tRep[OCRepresentation]: key=%s, value=%s\n",
                attrname.c_str(), "OCRep");
      PrintfOcRepresentation(cur.getValue<OCRepresentation>());
    } else if (AttributeType::Vector == cur.type()) {
      DEBUG_MSG("\tRep[OCRepresentation]: key=%s, value=%s\n",
                attrname.c_str(), "Vector");
      if (cur.base_type() == AttributeType::OCRepresentation) {
        DEBUG_MSG("\tVector of OCRepresentation\n");

        std::vector<OCRepresentation> v =
          cur.getValue<std::vector<OCRepresentation>>();
        for (const auto& curv : v) {
          DEBUG_MSG("\t>>Print sub OCRepresentation\n");
          PrintfOcRepresentation(curv);
          DEBUG_MSG("\t<<Print sub OCRepresentation\n");
        }
      }
    }
  }
  DEBUG_MSG("<<PrintfOcRepresentation:\n");
}

static void UpdateDestOcRepresentationString(OCRepresentation& oCReprDest,
    std::string attributeName,
    std::string value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::String == cur.type()) {
        std::string oldValue = cur.getValue<string>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%s, new=%s\n", attrname.c_str(),
                  oldValue.c_str(), value.c_str());
      } else {
        DEBUG_MSG("Updated[%s] mismatch String\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationInt(OCRepresentation& oCReprDest,
    std::string attributeName,
    int value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::Integer == cur.type()) {
        int oldValue = cur.getValue<int>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%d, new=%d\n", attrname.c_str(), oldValue,
                  value);
      } else if (AttributeType::Double == cur.type()) {
        // Force int to double
        double oldValue = cur.getValue<double>();
        cur = value;
        DEBUG_MSG("F-Updated[%s] old=%f, new=%d\n", attrname.c_str(), oldValue,
                  value);
      } else {
        DEBUG_MSG("Updated[%s] mismatch Int\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationBool(OCRepresentation& oCReprDest,
    std::string attributeName,
    bool value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::Boolean == cur.type()) {
        bool oldValue = cur.getValue<bool>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%d, new=%d\n", attrname.c_str(), oldValue,
                  value);
      } else {
        DEBUG_MSG("Updated[%s] mismatch Bool\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationDouble(OCRepresentation& oCReprDest,
    std::string attributeName,
    double value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::Double == cur.type()) {
        double oldValue = cur.getValue<double>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%f, new=%f\n", attrname.c_str(), oldValue,
                  value);
      } else if (AttributeType::Integer == cur.type()) {
        int oldValue = cur.getValue<int>();
        cur = value;
        DEBUG_MSG("F-Updated[%s] old=%d, new=%f\n", attrname.c_str(), oldValue,
                  value);
      } else {
        DEBUG_MSG("Updated[%s] mismatch Double\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationOCRep(OCRepresentation& oCReprDest,
    std::string attributeName,
    OCRepresentation& value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::OCRepresentation == cur.type()) {
        OCRepresentation oldValue = cur.getValue<OCRepresentation>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%s, new=%s\n", attrname.c_str(),
                  oldValue.getUri().c_str(),
                  value.getUri().c_str());
      } else {
        DEBUG_MSG("Updated[%s] mismatch OCRepresentation\n", attrname.c_str());
        return;
      }
    }
  }
}

void UpdateOcRepresentation(const OCRepresentation& oCReprSource,
                            OCRepresentation& oCReprDest,
                            std::vector<std::string>& updatedPropertyNames) {
  DEBUG_MSG("\n\nUpdateOcRepresentation: Source\n");
  PrintfOcRepresentation(oCReprSource);

  DEBUG_MSG("UpdateOcRepresentation: Destination\n");
  PrintfOcRepresentation(oCReprDest);

  std::vector<std::string> foundPropertyNames;
  for (auto& cur : oCReprSource) {
    std::string attrname = cur.attrname();

    DEBUG_MSG("SRC attrname=%s\n", attrname.c_str());

    AttributeValue destAttrValue;
    if (oCReprDest.getAttributeValue(attrname, destAttrValue) == false) {
      DEBUG_MSG("DST attrname=%s NOT FOUND !!!\n", attrname.c_str());
      continue;
    }

    if (std::find(updatedPropertyNames.begin(), updatedPropertyNames.end(),
                  attrname) != updatedPropertyNames.end()) {
      if (AttributeType::String == cur.type()) {
        DEBUG_MSG("cur.type String\n");
        std::string newValue = cur.getValue<string>();
        UpdateDestOcRepresentationString(oCReprDest, attrname, newValue);
      } else if (AttributeType::Integer == cur.type()) {
        DEBUG_MSG("cur.type Integer\n");
        int newValue = cur.getValue<int>();
        UpdateDestOcRepresentationInt(oCReprDest, attrname, newValue);
      } else if (AttributeType::Boolean == cur.type()) {
        DEBUG_MSG("cur.type Boolean\n");
        bool newValue = cur.getValue<bool>();
        UpdateDestOcRepresentationBool(oCReprDest, attrname, newValue);
      } else if (AttributeType::Double == cur.type()) {
        DEBUG_MSG("cur.type Double\n");
        double newValue = cur.getValue<double>();
        UpdateDestOcRepresentationDouble(oCReprDest, attrname, newValue);
      } else if (AttributeType::OCRepresentation == cur.type()) {
        DEBUG_MSG("cur.type OcRepresentation\n");
        OCRepresentation newValue = cur.getValue<OCRepresentation>();
        UpdateDestOcRepresentationOCRep(oCReprDest, attrname, newValue);
      } else if (AttributeType::Vector == cur.type()) {
        DEBUG_MSG("cur.type Vector\n");

        if (AttributeType::Integer == cur.base_type()) {
          std::vector<int> newValue = cur.getValue<std::vector<int>>();

          for (auto& curd : oCReprDest) {
            if (attrname != curd.attrname()) continue;
            if (AttributeType::Vector != curd.type()) continue;
            if (AttributeType::Integer != cur.base_type()) continue;
            curd = newValue;
          }
        } else if (AttributeType::Double == cur.base_type()) {
          std::vector<double> newValue = cur.getValue<std::vector<double>>();

          for (auto& curd : oCReprDest) {
            if (attrname != curd.attrname()) continue;
            if (AttributeType::Vector != curd.type()) continue;
            if (AttributeType::Double != cur.base_type()) continue;
            curd = newValue;
          }
        } else if (AttributeType::String == cur.base_type()) {
          std::vector<std::string> newValue =
            cur.getValue<std::vector<std::string>>();

          for (auto& curd : oCReprDest) {
            if (attrname != curd.attrname()) continue;
            if (AttributeType::Vector != curd.type()) continue;
            if (AttributeType::String != cur.base_type()) continue;
            curd = newValue;
          }
        } else if (AttributeType::OCRepresentation == cur.base_type()) {
          std::vector<OCRepresentation> newValue =
            cur.getValue<std::vector<OCRepresentation>>();

          for (auto& curd : oCReprDest) {
            if (attrname != curd.attrname()) continue;
            if (AttributeType::Vector != curd.type()) continue;
            if (AttributeType::OCRepresentation != cur.base_type()) continue;
            curd = newValue;
          }
        }
      }
    }
  }
}

void PicojsonPropsToOCRep(
    OCRepresentation &rep,
    picojson::object &props) {
  DEBUG_MSG(">>PicojsonPropsToOCReps \n");
  for (picojson::value::object::iterator piter = props.begin();
     piter != props.end(); ++piter) {
    std::string key = piter->first;
    picojson::value value = piter->second;

    DEBUG_MSG("\t>>key = %s\n", key.c_str());

    if (value.is<bool>()) {
       DEBUG_MSG("\tbool val\n");
      rep[key] = value.get<bool>();
    } else if (value.is<int>()) {
      DEBUG_MSG("\tint val\n");
      rep[key] = static_cast<int>(value.get<double>());
    } else if (value.is<double>()) {
      DEBUG_MSG("\tdouble val\n");
      rep[key] = value.get<double>();
    } else if (value.is<string>()) {
      DEBUG_MSG("\tstring val\n");
      rep[key] = value.get<string>();
    } else if (value.is<picojson::array>()) {
      DEBUG_MSG("\tarray val\n");
      picojson::array array = value.get<picojson::array>();
      picojson::array::iterator iter = array.begin();

      if ((*iter).is<bool>()) {
        DEBUG_MSG("\t\tbool base_type\n");
        std::vector<bool> v;
        for (; iter != array.end(); ++iter) {
          if ((*iter).is<bool>()) {
            v.push_back((*iter).get<bool>());
          }
        }
        rep[key] = v;
      } else if ((*iter).is<int>()) {
        DEBUG_MSG("\t\tint base_type\n");
        std::vector<int> v;
        for (; iter != array.end(); ++iter) {
          if ((*iter).is<int>()) {
            v.push_back((*iter).get<int>());
          }
        }
        rep[key] = v;
      } else if ((*iter).is<double>()) {
        DEBUG_MSG("\t\tdouble base_type\n");
        std::vector<double> v;
        for (; iter != array.end(); ++iter) {
          if ((*iter).is<double>()) {
            v.push_back((*iter).get<double>());
          }
        }
        rep[key] = v;
      } else if ((*iter).is<string>()) {
        DEBUG_MSG("\t\tstring base_type\n");
        std::vector<std::string> v;
        for (; iter != array.end(); ++iter) {
          if ((*iter).is<string>()) {
            v.push_back((*iter).get<string>());
          }
        }
        rep[key] = v;
      } else if ((*iter).is<picojson::object>()) {
        DEBUG_MSG("\t\tobject base_type\n");
        std::vector<OCRepresentation> v;
        for (; iter != array.end(); ++iter) {
          OCRepresentation repi;
          picojson::object propi = (*iter).get<picojson::object>();
          PicojsonPropsToOCRep(repi, propi);
          v.push_back(repi);
        }
        rep[key] = v;
      }
    }
    DEBUG_MSG("\t<<key = %s\n", key.c_str());
  }
  DEBUG_MSG("<<PicojsonPropsToOCReps after:\n");
  PrintfOcRepresentation(rep);
}

// Translate OCRepresentation to picojson
void TranslateOCRepresentationToPicojson(const OCRepresentation& oCRepr,
    picojson::object& objectRes) {
  objectRes["uri"] = picojson::value(oCRepr.getUri());
  for (auto& cur : oCRepr) {
    std::string attrname = cur.attrname();

    if (AttributeType::String == cur.type()) {
      std::string curStr = cur.getValue<string>();
      objectRes[attrname] = picojson::value(curStr);
    } else if (AttributeType::Integer == cur.type()) {
      int intValue = cur.getValue<int>();
      objectRes[attrname] = picojson::value(static_cast<double>(intValue));
    } else if (AttributeType::Double == cur.type()) {
      double doubleValue = cur.getValue<double>();
      objectRes[attrname] = picojson::value(doubleValue);
    } else if (AttributeType::Boolean == cur.type()) {
      bool boolValue = cur.getValue<bool>();
      objectRes[attrname] = picojson::value(boolValue);
    } else if (AttributeType::OCRepresentation == cur.type()) {
      OCRepresentation ocrValue = cur.getValue<OCRepresentation>();
      picojson::object picoRep;
      TranslateOCRepresentationToPicojson(ocrValue, picoRep);
      objectRes[attrname] = picojson::value(picoRep);
    } else if (AttributeType::Vector == cur.type()) {
      DEBUG_MSG("\tTranslateArrayToPicojson\n");
      picojson::array array;

      if (cur.base_type() == AttributeType::OCRepresentation) {
        std::vector<OCRepresentation> v =
          cur.getValue<std::vector<OCRepresentation>>();

        for (auto const item : v) {
          picojson::object obj;
          TranslateOCRepresentationToPicojson(item, obj);
          array.push_back(picojson::value(obj));
        }
      } else if (cur.base_type() == AttributeType::String) {
        std::vector<std::string> v = cur.getValue<std::vector<std::string>>();
        for (auto const item : v) {
          array.push_back(picojson::value(item));
        }
      } else if (cur.base_type() == AttributeType::Boolean) {
        std::vector<bool> v = cur.getValue<std::vector<bool>>();
        for (auto const item : v) {
          array.push_back(picojson::value(item));
        }
      } else if (cur.base_type() == AttributeType::Double) {
        std::vector<double> v = cur.getValue<std::vector<double>>();
        for (auto const item : v) {
          array.push_back(picojson::value(item));
        }
      } else if (cur.base_type() == AttributeType::Integer) {
        std::vector<int> v = cur.getValue<std::vector<int>>();
        for (auto const item : v) {
          array.push_back(picojson::value(static_cast<double>(item)));
        }
      }
      objectRes[attrname] = picojson::value(array);
    }
  }
}

void CopyInto(std::vector<std::string>& src, picojson::array& dest) {
  for (unsigned int i = 0; i < src.size(); i++) {
    std::string str = src[i];
    dest.push_back(picojson::value(str));
  }
}

int GetWait(picojson::value value) {
  picojson::value param = value.get("OicDiscoveryOptions");
  int waitsec = 5;

  if (param.contains("waitsec")) {
    waitsec = static_cast<int>(param.get("waitsec").get<double>());
  }

  return waitsec;
}
