/*
    SchemaValidator.cc -- apply JSON Schema
    Copyright 2010 The Chromium Authors. All rights reserved.
    Copyright 2015-2018 nfotex IT DL GmbH.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    3. Neither the name Google Inc., nfotex IT DL GmbH, nor the names of
       its contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define __STDC_FORMAT_MACROS

#include <json/SchemaValidator.h>

#include <inttypes.h>
#include <stdio.h>

#include <cfloat>
#include <cmath>
#include <exception>
#include <limits>

#include <pcrecpp.h>

#include <json/Pointer.h>

#undef JSON_DEBUG_REF

namespace Json {
#if 0
} // fix auto indent
#endif

SchemaValidator *SchemaValidator::meta_validator = NULL;
Json::Value SchemaValidator::meta_schema_root;

const std::vector<std::string> SchemaValidator::schema_member_names = {
  "additionalItems",
  "additionalProperties",
  "contains",
  "else",
  "if",
  "items",
  "not",
  "propertyNames",
  "then"
};

const std::vector<std::string> SchemaValidator::schema_array_member_names = {
  "allOf",
  "anyOf",
  "items",
  "oneOf"
};

const std::vector<std::string> SchemaValidator::schema_object_member_names = {
  "definitions",
  "dependencies",
  "patternProperties",
  "properties"
};


std::string SchemaValidator::Exception::type_message() {
  switch (type) {
    case Json::SchemaValidator::Exception::INTERNAL:
      return "internal error";
    
    case Json::SchemaValidator::Exception::PARSING:
      return "parse error";
    
    case Json::SchemaValidator::Exception::POINTER:
      return "invalid schema pointer";
      
    case Json::SchemaValidator::Exception::SCHEMA_VALIDATION:
      return "invalid schema";
  }
  
  return "unknown error";
}

static void ReplaceFirstSubstringAfterOffset(std::string* str,
                                             std::string::size_type start_offset,
                                             const std::string& find_this,
                                             const std::string& replace_with) {
  if ((start_offset == std::string::npos) || (start_offset >= str->length()))
    return;

  if (find_this.empty())
      return;
    
  auto offs = str->find(find_this, start_offset);
  if (offs != std::string::npos) {
    str->replace(offs, find_this.length(), replace_with);
  }
}


static std::string IntToString(int i) {
  char buf[1024];

  sprintf(buf, "%d", i);
  return std::string(buf);
}
static std::string UIntToString(Json::UInt64 i) {
  char buf[1024];

  sprintf(buf, "%" PRIu64, i);
  return std::string(buf);
}
static std::string DoubleToString(double d) {
  char buf[1024];

  sprintf(buf, "%f", d);
  return std::string(buf);
}


SchemaValidator::Error::Error() {
}

SchemaValidator::Error::Error(const std::string& message_)
    : path(message_) {
}

SchemaValidator::Error::Error(const std::string& path_,
                                  const std::string& message_)
    : path(path_), message(message_) {
}


const char SchemaValidator::kUnknownTypeReference[] =
    "Unknown schema reference: *.";
const char SchemaValidator::kMismatchedSelfReference[] =
    "Schema self reference doesn't match registered schema for id *.";
const char SchemaValidator::kInvalidChoice[] =
    "Value does not match any valid type choices.";
const char SchemaValidator::kInvalidEnum[] =
    "Value does not match any valid enum choices.";
const char SchemaValidator::kObjectPropertyIsRequired[] =
    "Required property * is missing.";
const char SchemaValidator::kUnexpectedProperty[] =
    "Unexpected property.";
const char SchemaValidator::kObjectMinProperties[] =
    "Object must have at least * properties.";
const char SchemaValidator::kObjectMaxProperties[] =
    "Object must not have more than * properties.";
const char SchemaValidator::kArrayMinItems[] =
    "Array must have at least * items.";
const char SchemaValidator::kArrayMaxItems[] =
    "Array must not have more than * items.";
const char SchemaValidator::kArrayItemRequired[] =
    "Item is required.";
const char SchemaValidator::kArrayItemsNotUnique[] =
    "Items not unique.";
const char SchemaValidator::kNoAdditionalItems[] =
    "Additional items not allowed.";
const char SchemaValidator::kStringMinLength[] =
    "String must be at least * characters long.";
const char SchemaValidator::kStringMaxLength[] =
    "String must not be more than * characters long.";
const char SchemaValidator::kStringPattern[] =
    "String must match the pattern: *.";
const char SchemaValidator::kNumberMinimum[] =
    "Value must not be less than *.";
const char SchemaValidator::kNumberMaximum[] =
    "Value must not be greater than *.";
const char SchemaValidator::kNumberExclusiveMinimum[] =
    "Value must be greater than *.";
const char SchemaValidator::kNumberExclusiveMaximum[] =
    "Value must be less than *.";
const char SchemaValidator::kNumberDivisible[] =
    "Value must be multiple of *.";
const char SchemaValidator::kInvalidType[] =
    "Expected '*' but got '*'.";
const char SchemaValidator::kNotNegative[] =
    "Parameter * must not be less than 0";
const char SchemaValidator::kEmptyType[] =
    "Type is empty string";
const char SchemaValidator::kAnyOfFailed[] =
    "None of the option schemata was matched.";
const char SchemaValidator::kOneOfFailed[] =
    "Not exactly one of the option schemata was matched.";
const char SchemaValidator::kNotFailed[] =
    "Disallowed schema was matched.";
const char SchemaValidator::kFalse[] =
    "Schema false always fails.";
const char SchemaValidator::kArrayContains[] =
    "Array does not contain matching item.";
const char SchemaValidator::kConst[] =
    "Value does not match const.";


// static
std::string SchemaValidator::GetSchemaType(const Json::Value &value) {
  switch (value.type()) {
    case Json::nullValue:
      return "null";
    case Json::booleanValue:
      return "boolean";
    case Json::intValue:
    case Json::uintValue:
      return "integer";
    case Json::realValue: {
      double double_value = value.asDouble();
      if (std::abs(double_value) <= std::pow(2.0, DBL_MANT_DIG) &&
          double_value == floor(double_value)) {
        return "integer";
      } else {
        return "number";
      }
    }
    case Json::stringValue:
      return "string";
    case Json::objectValue:
      return "object";
    case Json::arrayValue:
      return "array";
    default:
      //CHECK(false); //<< "Unexpected value type: " << value->GetType();
      return "";
  }
}

// static
std::string SchemaValidator::FormatErrorMessage(const std::string& format,
                                                    const std::string& s1) {
  std::string ret_val = format;
  ReplaceFirstSubstringAfterOffset(&ret_val, 0, "*", s1);
  return ret_val;
}

// static
std::string SchemaValidator::FormatErrorMessage(const std::string& format,
                                                    const std::string& s1,
                                                    const std::string& s2) {
  std::string ret_val = format;
  ReplaceFirstSubstringAfterOffset(&ret_val, 0, "*", s1);
  ReplaceFirstSubstringAfterOffset(&ret_val, 0, "*", s2);
  return ret_val;
}

SchemaValidator::SchemaValidator(std::string schema_string, const Options &options) {
  Json::Reader reader;

  bool success = reader.parse (schema_string, refs_root_);
  if (!success) {
    SchemaValidator::Exception e(Exception::PARSING);
    // TODO: properly formatted error messages once we have the interface
    e.errors.push_back(Error("", reader.getFormattedErrorMessages()));
    throw e;
  }
  
  init(options, true);
}


SchemaValidator *SchemaValidator::create_meta_validator() {
  if (meta_schema_root.isNull()) {
    Json::Reader reader;
    
    if (!reader.parse(meta_schema, meta_schema_root)) {
      throw Exception(Exception::INTERNAL);
    }
  }
  
  return new SchemaValidator(meta_schema_root, Options(), false);
}

SchemaValidator::SchemaValidator(Json::Value schema, const Options &options, bool validate_schema) : refs_root_(schema) {
  init(options, validate_schema);
}

void SchemaValidator::init(const Options &options, bool validate_schema) {
  if (options.schema_pointer.length() > 0) {
    try {
      Json::Pointer pointer(options.schema_pointer);
      schema_root_ = &pointer.get(refs_root_);
    } catch (std::exception &ex) {
      SchemaValidator::Exception e(Exception::POINTER);
      e.errors.push_back(Error("", ex.what()));
      throw e;
    }
  }
  else {
    schema_root_ = &refs_root_;
  }
  
  if (validate_schema) {
    if (meta_validator == NULL) {
      meta_validator = create_meta_validator();
    }
    
    if (!meta_validator->validate(*schema_root_)) {
      SchemaValidator::Exception e(Exception::SCHEMA_VALIDATION);
      if (options.schema_pointer.length() > 0) {
        e.errors = meta_validator->errors(options.schema_pointer);
      }
      else {
        e.errors = meta_validator->errors();
      }
      throw e;
    }
  }

#ifdef JSON_DEBUG_REF
  printf("init: root: %p, schema: %p\n", &refs_root_, schema_root_);
#endif
  if (refs_root_.isObject() && !refs_root_.isMember("$id")) {
    ids[""] = &refs_root_;
  }
  if (&refs_root_ != schema_root_ && refs_root_.isMember("definitions")) {
    const Json::Value &definitions = refs_root_["definitions"];
    for (auto key : definitions.getMemberNames()) {
      const Json::Value &schema = definitions[key];
      if (validate_schema) {
        if (!meta_validator->validate(schema)) {
          SchemaValidator::Exception e(Exception::SCHEMA_VALIDATION);
          e.errors = meta_validator->errors("/definitions/" + key);
          throw e;
        }
      }
      collect_ids_refs(schema, URI(), false);
      collect_ids_refs(schema, URI(), true);
    }
  }

  collect_ids_refs(*schema_root_, URI(), false);
  collect_ids_refs(*schema_root_, URI(), true);

  std::unordered_set<const Json::Value *> new_sub_schemata;
  do {
    new_sub_schemata.clear();
    for (auto pair : refs) {
      if (sub_schemata.find(pair.second) == sub_schemata.end()) {
        if (validate_schema) {
          if (!meta_validator->validate(*pair.second)) {
            SchemaValidator::Exception e(Exception::SCHEMA_VALIDATION);
            e.errors = meta_validator->errors(); // TODO: don't know path
            throw e;
          }
        }
        new_sub_schemata.insert(pair.second);
      }
    }

    for (auto node : new_sub_schemata) {
      // TODO: don't know correct URI.
      collect_ids_refs(*node, URI(), false);
      collect_ids_refs(*node, URI(), true);
    }
  } while (!new_sub_schemata.empty());

  for (auto pair : refs) {
    const Json::Value *node = pair.second;
    if (node->isObject() && node->isMember("$ref")) {
      std::unordered_set<const Json::Value *> nodes_seen;
      nodes_seen.insert(pair.first);
      const Json::Value *target = node;
      while (target->isObject() && target->isMember("$ref")) {
        if (nodes_seen.find(target) != nodes_seen.end()) {
          SchemaValidator::Exception e(Exception::SCHEMA_VALIDATION);
          e.errors.push_back(Error("","reference loop including '" + (*pair.first)["$ref"].asString() + "'"));
          throw e;
        }
        nodes_seen.insert(target);
        auto it = refs.find(target);
        if (it == refs.end()) {
          SchemaValidator::Exception e(Exception::INTERNAL);
          e.errors.push_back(Error("","unresolved reference '" + (*target)["$ref"].asString() + "'"));
          throw e;
        }
        target = it->second;
      }
      for (auto source : nodes_seen) {
        refs[source] = target;
      }
    }
  }

  ids.clear();
  sub_schemata.clear();
}


void SchemaValidator::collect_ids_refs(const Json::Value &node, URI base_uri, bool process_refs) {
  if (!process_refs) {
    sub_schemata.insert(&node);
  }

  if (!node.isObject()) {
    return;
  }
  if (node.isMember("$ref")) {
    if (process_refs) {
      auto ref_uri = base_uri.resolve(node["$ref"].asString());
      auto ref_string = ref_uri.get_uri();
      std::string fragment;

      if (ref_uri.has_fragment()) {
        fragment = ref_uri.get_fragment();
      }

      if (fragment.empty()) {
        ref_uri.clear_fragment();
      }
      else {
        if (fragment[0] == '/') {
          ref_uri.clear_fragment();
        }
        else {
          fragment = "";
        }
      }

      const Json::Value *ref_node = NULL;

      if (ref_uri.get_uri().empty()) {
        ref_node = &refs_root_;
      }
      else {
        auto it = ids.find(ref_uri.get_uri());

        if (it == ids.end()) {
          SchemaValidator::Exception e(Exception::POINTER);
          // TODO: more details in error message?
          e.errors.push_back(Error("", "unresolved ref " + ref_string));
          throw e;
        }

        ref_node = it->second;
      }

      if (!fragment.empty()) {
        try {
          Pointer pointer(fragment);
          const Json::Value &obj = pointer.get(*ref_node);
          ref_node = &obj;
        }
        catch (std::exception &ex) {
          SchemaValidator::Exception e(Exception::POINTER);
          // TODO: more details in error message?
          e.errors.push_back(Error("", ex.what()));
          throw e;
        }
      }
#ifdef JSON_DEBUG_REF
      printf("  (%p) recording ref %s -> %p\n", &node, ref_string.c_str(), ref_node);
#endif
      refs[&node] = ref_node;
    }
  }
  else if (node.isMember("$id")) {
    base_uri = base_uri.resolve(node["$id"].asString());
    if (base_uri.has_fragment() && base_uri.get_fragment().empty()) {
      base_uri.clear_fragment();
    }
    if (!process_refs) {
#ifdef JSON_DEBUG_REF
      printf("  id %s -> %p\n", base_uri.get_uri().c_str(), &node);
#endif
      ids[base_uri.get_uri()] = &node;
    }
  }

  for (auto key : schema_member_names) {
    if (node.isMember(key) && node[key].isObject()) {
      collect_ids_refs(node[key], base_uri, process_refs);
    }
  }

  for (auto key : schema_array_member_names) {
    if (node.isMember(key) && node[key].isArray()) {
      for (const Json::Value &child: node[key]) {
        collect_ids_refs(child, base_uri, process_refs);
      }
    }
  }

  for (auto key : schema_object_member_names) {
    if (node.isMember(key)) {
      const Json::Value &member = node[key];
      for (auto name : member.getMemberNames()) {
        collect_ids_refs(member[name], base_uri, process_refs);
      }
    }
  }
}


SchemaValidator::~SchemaValidator() {}

std::vector<SchemaValidator::Error> SchemaValidator::errors(std::string prefix) const {
  auto orig_errors = errors();
  std::vector<SchemaValidator::Error> prefixed_errors;
  for (auto it = orig_errors.begin(); it != orig_errors.end(); it++) {
    prefixed_errors.push_back(Error(prefix + it->path, it->message));
  }
  return prefixed_errors;
}


bool SchemaValidator::validate(const Json::Value &instance) {
  /// \todo fix non-relative $ref
  /// \todo walk schema and record all ids in types_
  /// \todo get rid of asserts (CHECK)?
#ifdef JSON_DEBUG_REF
  printf("validate: root: %p, schema: %p, instance %p\n", &refs_root_, schema_root_, &instance);
#endif
  errors_.clear();
  Validate(instance, *schema_root_, "/");
  return errors_.size() == 0;
}


bool SchemaValidator::isValid(const Json::Value &instance, const Json::Value &schema) {
  auto errors_before = errors_.size();
  Validate(instance, schema, "");
  auto ok = errors_.size() == errors_before;
  errors_.resize(errors_before);
  return ok;
}

void SchemaValidator::Validate(const Json::Value &instance,
                               const Json::Value &schema,
                               const std::string& path) {
  if (schema.isBool()) {
      if (schema.asBool() == false) {
        errors_.push_back(Error(path, kFalse));
      }
      return;
  }

  // If the schema has a $ref property, the instance must validate against
  // that schema. It must be present in types_ to be referenced.
  if (schema.isMember("$ref")) {
    auto it = refs.find(&schema);
    if (it == refs.end()) {
#ifdef JSON_DEBUG_REF
      printf("  (%p) unresolved ref %s\n", &schema, schema["$ref"].asCString());
#endif
      // should not happen
      errors_.push_back(Error(path, FormatErrorMessage(kUnknownTypeReference, schema["$ref"].asString())));
    }
    else {
#ifdef JSON_DEBUG_REF
      printf("  (%p) looking up ref %s -> %p\n", &schema, schema["$ref"].asCString(), it->second);
#endif
      Validate(instance, *(it->second), path);
    }
    return;
  }

  // If the schema has a choices property, the instance must validate against at
  // least one of the items in that array.
  if (schema.isMember("type")) {
    if (!ValidateType(instance, schema["type"], path)) {
      return;
    }
  }

  if (schema.isMember("allOf")) {
    const Json::Value &schemata = schema["allOf"];

    for (Json::ArrayIndex i = 0; i < schemata.size(); i++) {
      Validate(instance, schemata[i], path);
    }
  }
  if (schema.isMember("anyOf")) {
    const Json::Value &schemata = schema["anyOf"];
    bool ok = false;
    
    for (Json::ArrayIndex i = 0; i < schemata.size(); i++) {
      if (isValid(instance, schemata[i])) {
        ok = true;
        break;
      }
    }

    if (!ok) {
      errors_.push_back(Error(path, kAnyOfFailed));
    }
  }
  if (schema.isMember("oneOf")) {
    const Json::Value &schemata = schema["oneOf"];
    size_t matched = 0;
    
    for (Json::ArrayIndex i = 0; i < schemata.size(); i++) {
      if (isValid(instance, schemata[i])) {
        matched++;
      }
    }
    
    if (matched != 1) {
      errors_.push_back(Error(path, kOneOfFailed));
    }
  }
  if (schema.isMember("not")) {
    if (isValid(instance, schema["not"])) {
      errors_.push_back(Error(path, kNotFailed));
    }
  }

  if (schema.isMember("if") && (schema.isMember("then") || schema.isMember("else"))) {
    if (isValid(instance, schema["if"])) {
      if (schema.isMember("then")) {
        Validate(instance, schema["then"], path);
      }
    }
    else {
      if (schema.isMember("else")) {
        Validate(instance, schema["else"], path);
      }
    }
  }

  if (schema.isMember("const")) {
    if (instance != schema["const"]) {
      errors_.push_back(Error(path, kConst));
    }
  }
  // If the schema has an enum property, the instance must be one of those
  // values.
  if (schema.isMember("enum")) {
    ValidateEnum(instance, schema["enum"], path);
    return;
  }

  if (instance.isNull() || instance.isBool())
    return;
  else if (instance.isObject())
    ValidateObject(instance, schema, path);
  else if (instance.isArray())
    ValidateArray(instance, schema, path);
  else if (instance.isString())
    ValidateString(instance, schema, path);
  else if (instance.isNumeric())
    ValidateNumber(instance, schema, path);
}

bool SchemaValidator::ValidateChoices(const Json::Value &instance,
                                          const Json::Value &choices,
                                          const std::string& path) {
    size_t original_num_errors = errors_.size();

    for (Json::Value::ArrayIndex i = 0; i < choices.size(); ++i) {
        if (ValidateSimpleType(instance, choices[i].asString(), path)) {
            return true;
        }
        // We discard the error from each choice. We only want to know if any of the
        // validations succeeded.
        errors_.resize(original_num_errors);
    }

    // TODO: better error message
    // Now add a generic error that no choices matched.
    errors_.push_back(Error(path, kInvalidChoice));
    return false;
}

void SchemaValidator::ValidateEnum(const Json::Value &instance,
                                       const Json::Value &choices,
                                       const std::string& path) {
  for (Json::Value::ArrayIndex i = 0; i < choices.size(); ++i) {
    if (choices[i] == instance) {
      return;
    }
  }

  errors_.push_back(Error(path, kInvalidEnum));
}

void SchemaValidator::ValidateObject(const Json::Value &instance, const Json::Value &schema, const std::string& path) {
  if (schema.isMember("required")) {
    const Json::Value &required = schema["required"];
    for (Json::ArrayIndex i = 0; i < required.size(); i++) {
      if (!instance.isMember(required[i].asString())) {
        errors_.push_back(Error(path, FormatErrorMessage(kObjectPropertyIsRequired, required[i].asString())));
      }
    }
  }
  
  if (schema.isMember("minProperties")) {
    Json::UInt64 count = schema["minProperties"].asUInt();
    
    if (instance.size() < count) {
      errors_.push_back(Error(path, FormatErrorMessage(kObjectMinProperties, UIntToString(count))));
    }
  }

  if (schema.isMember("maxProperties")) {
    Json::UInt64 count = schema["maxProperties"].asUInt();
    
    if (instance.size() > count) {
      errors_.push_back(Error(path, FormatErrorMessage(kObjectMaxProperties, UIntToString(count))));
    }
  }

  const Json::Value *properties = schema.isMember("properties") ? &schema["properties"] : NULL;
  const Json::Value *property_names = schema.isMember("propertyNames") ? &schema["propertyNames"] : NULL;
  const Json::Value *additional_properties = schema.isMember("additionalProperties") ? &schema["additionalProperties"] : NULL;
  std::vector<std::pair<pcrecpp::RE, const Json::Value *> >pattern_properties;
  if (schema.isMember("patternProperties")) {
    const Json::Value &pattern = schema["patternProperties"];
    for (auto name : pattern.getMemberNames()) {
      pattern_properties.push_back(std::pair<pcrecpp::RE, const Json::Value *>(pcrecpp::RE(name), &pattern[name]));
    }
  }
  const Json::Value *dependencies = schema.isMember("dependencies") ? &schema["dependencies"] : NULL;

  for (auto name : instance.getMemberNames()) {
    auto checked = false;

    const Json::Value &child = instance[name];
    auto child_path = path_add(path, name);

    if (property_names != NULL) {
      Validate(name, *property_names, child_path);
    }

    if (properties && properties->isMember(name)) {
      Validate(child, (*properties)[name], child_path);
      checked = true;
    }

    for (auto pair : pattern_properties) {
      if (pair.first.PartialMatch(name)) {
        Validate(child, *(pair.second), child_path);
        checked = true;
      }
    }

    if (!checked && additional_properties != NULL) {
      if (additional_properties->isBool() && additional_properties->asBool() == false) {
        errors_.push_back(Error(child_path, kUnexpectedProperty));
      }
      else {
        Validate(child, *additional_properties, child_path);
      }
    }

    if (dependencies != NULL && dependencies->isMember(name)) {
      const Json::Value &dependency = (*dependencies)[name];
      if (dependency.isArray()) {
        for (auto dependency_name : dependency) {
          if (!instance.isMember(dependency_name.asString())) {
            errors_.push_back(Error(path, FormatErrorMessage(kObjectPropertyIsRequired, dependency_name.asString())));
          }
        }
      }
      else {
        Validate(instance, dependency, path);
      }
    }
  }
}

void SchemaValidator::ValidateArray(const Json::Value &instance, const Json::Value &schema, const std::string& path) {
  Json::ArrayIndex instance_size = instance.size();

  if (schema.isMember("minItems")) {
    int min_items = schema["minItems"].asInt();

    if (instance_size < static_cast<size_t>(min_items)) {
      errors_.push_back(Error(path, FormatErrorMessage(kArrayMinItems, IntToString(min_items))));
    }
  }

  if (schema.isMember("maxItems")) {
    int max_items = schema["maxItems"].asInt();
    if (instance_size > static_cast<size_t>(max_items)) {
      errors_.push_back(Error(path, FormatErrorMessage(kArrayMaxItems, IntToString(max_items))));
    }
  }
  
  Json::ArrayIndex items_size = std::numeric_limits<Json::ArrayIndex>::max();
  
  if (schema.isMember("items")) {
    const Json::Value &items = schema["items"];

    if (items.isArray()) {
      items_size = items.size();
      for (Json::ArrayIndex i = 0; i < items_size && i < instance_size; ++i) {
        Validate(instance[i], items[i], path_add(path, UIntToString(i)));
      }
    }
    else {
      // If the items property is a single schema, each item in the array must
      // validate against that schema.
      for (Json::ArrayIndex i = 0; i < instance_size; ++i) {
        Validate(instance[i], items, path_add(path, UIntToString(i)));
      }
      return;
    }

    if (instance_size > items_size) {
      if (schema.isMember("additionalItems")) {
        const Json::Value &additional = schema["additionalItems"];
      
        if (additional.isBool()) {
          if (!additional.asBool()) {
            errors_.push_back(Error(path, kNoAdditionalItems));
          }
        }
        else {
          for (Json::ArrayIndex i = items_size; i < instance_size; ++i) {
            Validate(instance[i], additional, path_add(path, UIntToString(i)));
          }
        }
      }
    }
  }

  if (schema.isMember("uniqueItems") && schema["uniqueItems"].asBool()) {
    for (Json::ArrayIndex i=0; i<instance.size(); i++) {
      for (Json::ArrayIndex j=i+1; j<instance.size(); j++) {
        if (instance[i] == instance[j])
          errors_.push_back(Error(path, kArrayItemsNotUnique));
      }
    }
  }

  if (schema.isMember("contains")) {
    auto ok = false;
    const Json::Value &contains_schema = schema["contains"];

    for (auto item : instance) {
      if (isValid(item, contains_schema)) {
        ok = true;
        break;
      }
    }

    if (!ok) {
      errors_.push_back(Error(path, kArrayContains));
    }
  }
}

void SchemaValidator::ValidateString(const Json::Value &instance,
                                         const Json::Value &schema,
                                         const std::string& path) {
  const std::string &value = instance.asString();

  if (schema.isMember("minLength") || schema.isMember("maxLength")) {
    size_t length = count_utf8_characters(value);

    if (schema.isMember("minLength")) {
      int min_length = schema["minLength"].asInt();
      if (min_length < 0) {
        errors_.push_back(Error(path, FormatErrorMessage(kNotNegative, "minLength")));
        return;
      }

      if (length < static_cast<size_t>(min_length)) {
        errors_.push_back(Error(path, FormatErrorMessage(kStringMinLength, IntToString(min_length))));
      }
    }

    if (schema.isMember("maxLength")) {
      int max_length = schema["maxLength"].asInt();
      if (max_length < 0) {
        errors_.push_back(Error(path, FormatErrorMessage(kNotNegative, "maxLength")));
        return;
      }

      if (length > static_cast<size_t>(max_length)) {
        errors_.push_back(Error(path, FormatErrorMessage(kStringMaxLength, IntToString(max_length))));
      }
    }
  }

  if (schema.isMember("pattern")) {
    std::string pattern = schema["pattern"].asString();
    if (!pcrecpp::RE(pattern).PartialMatch(instance.asString()))
      errors_.push_back(Error(path, FormatErrorMessage(kStringPattern, pattern)));
  }
}

void SchemaValidator::ValidateNumber(const Json::Value &instance,
                                         const Json::Value &schema,
                                         const std::string& path) {
  double value = instance.asDouble();

  // TODO(aa): It would be good to test that the double is not infinity or nan,
  // but isnan and isinf aren't defined on Windows.

  if (schema.isMember("minimum")) {
    double minimum = schema["minimum"].asDouble();

    if (value < minimum) {
      errors_.push_back(Error(path, FormatErrorMessage(kNumberMinimum, DoubleToString(minimum))));
    }
  }

  if (schema.isMember("exclusiveMinimum")) {
    double minimum = schema["exclusiveMinimum"].asDouble();

    if (value <= minimum) {
      errors_.push_back(Error(path, FormatErrorMessage(kNumberExclusiveMinimum, DoubleToString(minimum))));
    }
  }

  if (schema.isMember("maximum")) {
    double maximum = schema["maximum"].asDouble();

    if (value > maximum) {
      errors_.push_back(Error(path, FormatErrorMessage(kNumberMaximum, DoubleToString(maximum))));
    }
  }

  if (schema.isMember("exclusiveMaximum")) {
    double maximum = schema["exclusiveMaximum"].asDouble();

    if (value >= maximum) {
      errors_.push_back(Error(path, FormatErrorMessage(kNumberExclusiveMaximum, DoubleToString(maximum))));
    }
  }

  if (schema.isMember("multipleOf")) {
    double divisor = schema["multipleOf"].asDouble();
    
    if (divisor != 0. && floor(value/divisor) != (value/divisor)) {
      errors_.push_back(Error(path, FormatErrorMessage(
          kNumberDivisible, DoubleToString(divisor))));
    }
  }
}

bool SchemaValidator::ValidateType(const Json::Value &instance, const Json::Value& type,
                                       const std::string& path) {
  if (type.isArray())
    return ValidateChoices(instance, type, path);

  std::string simple_type = type.asString();
  if (simple_type.empty()) {
      errors_.push_back(Error(path, kEmptyType));
      return false;
  }
  return ValidateSimpleType(instance, simple_type, path);
}

bool SchemaValidator::ValidateSimpleType(const Json::Value &instance,
                                             const std::string& expected_type,
                                             const std::string& path) {
  std::string actual_type = GetSchemaType(instance);
  if (expected_type == actual_type ||
      (expected_type == "number" && actual_type == "integer")) {
    return true;
  } else {
    errors_.push_back(Error(path, FormatErrorMessage(
        kInvalidType, expected_type, actual_type)));
    return false;
  }
}


std::string SchemaValidator::path_add(const std::string &path, const std::string &element) {
    if (path.length() != 1) {
        return path + "/" + element;
    }
    return path + element;
}

size_t SchemaValidator::count_utf8_characters(const std::string &str) {
  size_t length = 0;
  
  for (size_t i = 0; i < str.length(); i++) {
    if (static_cast<unsigned char>(str[i]) < 0x80 || static_cast<unsigned char>(str[i]) >= 0xc0) {
      length++;
    }
  }
  
  return length;
}

} // namespace nfotex_nsl
