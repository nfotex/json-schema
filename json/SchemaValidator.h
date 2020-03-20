/*
    SchemaValidator.h -- apply JSON Schema
    Copyright 2010 The Chromium Authors. All rights reserved.
    Copyright 2015-2020 nfotex IT DL GmbH.

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

#ifndef JSON_SCHEMA_VALIDATOR_H
#define JSON_SCHEMA_VALIDATOR_H

#include <stdarg.h>

#include <exception>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <json/json.h>
#include <json/URI.h>

namespace Json {
#if 0
} // fix auto indent
#endif

class SchemaValidator {
    class Options {
    public:
        Options()  { }
        Options(const std::string &pointer) : schema_pointer(pointer) { }
        
        std::string schema_pointer;
    };
    
 public:
  class ExpansionOptions {
  public:
      ExpansionOptions(bool add_defaults_ = false) : add_defaults(add_defaults_) { }
      
      bool add_defaults;
  };
  
  // Details about a validation error.
  struct Error {
    Error();

    explicit Error(const std::string& message);

    Error(const std::string& path, const std::string& message);

    // The path to the location of the error in the JSON structure.
    std::string path;

    // An english message describing the error.
    std::string message;
  };

  class Exception {
  public:
    enum Type {
      INTERNAL,
      PARSING,
      POINTER,
      SCHEMA_VALIDATION
    };
    
    Exception (Type type_) : type(type_) { }
    virtual ~Exception() throw() { }
    
    std::string type_message();
    
    Type type;
    std::vector<SchemaValidator::Error> errors;
  };

  // Error messages.
  static const char kUnknownTypeReference[];
  static const char kMismatchedSelfReference[];
  static const char kInvalidChoice[];
  static const char kInvalidEnum[];
  static const char kObjectPropertyIsRequired[];
  static const char kUnexpectedProperty[];
  static const char kObjectMinProperties[];
  static const char kObjectMaxProperties[];
  static const char kArrayMinItems[];
  static const char kArrayMaxItems[];
  static const char kArrayItemsNotUnique[];
  static const char kArrayItemRequired[];
  static const char kStringMinLength[];
  static const char kStringMaxLength[];
  static const char kStringPattern[];
  static const char kNumberMinimum[];
  static const char kNumberMaximum[];
  static const char kNumberExclusiveMinimum[];
  static const char kNumberExclusiveMaximum[];
  static const char kNumberDivisible[];
  static const char kInvalidType[];
  static const char kDisallowed[];
  static const char kNotNegative[];
  static const char kEmptyType[];
  static const char kNoAdditionalItems[];
  static const char kAnyOfFailed[];
  static const char kOneOfFailed[];
  static const char kNotFailed[];
  static const char kFalse[];
  static const char kArrayContains[];
  static const char kConst[];

  // Classifies a Value as one of the JSON schema primitive types.
  static std::string GetSchemaType(const Json::Value &value);

  // Utility methods to format error messages. The first method can have one
  // wildcard represented by '*', which is replaced with s1. The second method
  // can have two, which are replaced by s1 and s2.
  static std::string FormatErrorMessage(const std::string& format,
                                        const std::string& s1);
  static std::string FormatErrorMessage(const std::string& format,
                                        const std::string& s1,
                                        const std::string& s2);
    
    static SchemaValidator *create_meta_validator();

  /// Creates a validator for the specified schema.
  ///
  /// NOTE: This constructor assumes that |schema| is well formed and valid.
  /// Errors will result in CHECK at runtime; this constructor should not be used
  /// with untrusted schemas.
  explicit SchemaValidator(Json::Value schema, const Options &options = Options()) : SchemaValidator(schema, options, true) { }

  /// Creates a validator for the specified schema as JSON string.
  ///
  /// NOTE: This constructor assumes that |schema| is well formed and valid.
  /// Errors will result in CHECK at runtime; this constructor should not be used
  /// with untrusted schemas.
  explicit SchemaValidator(std::string schema_str, const Options &options = Options());

  ~SchemaValidator();

  /// Returns any errors from the last call to to Validate().
  const std::vector<Error>& errors() const {
    return errors_;
  }

    std::vector<Error> errors(std::string prefix) const;

  /// Validates a JSON value. Returns true if the instance is valid, false
  /// otherwise. If false is returned any errors are available from the errors()
  /// getter.
  bool validate(const Json::Value &instance);
    
  /// Validates a JSON value and expand according to options.
  ///  Returns true if the instance is valid, false otherwise.
  ///  If false is returned any errors are available from the errors() getter.
  bool validate_and_expand(Json::Value &instance, const ExpansionOptions &options);
    
  /// Validates a JSON value. Returns true if the instance is valid, false
  /// otherwise. If false is returned any errors are returned in errors.
  /// This variant is thread save: one validator can run multiple validations simultaneously.
  bool validate(const Json::Value &instance, std::vector<Error> *errors) const;

  /// Validates a JSON value and expand according to options.
  ///  Returns true if the instance is valid, false otherwise.
  ///  If false is returned any errors are returned in errors.
  /// This variant is thread save: one validator can run multiple validations simultaneously.
  bool validate_and_expand(Json::Value &instance, const ExpansionOptions &options, std::vector<Error> *errors) const;

 private:
    struct AddValue {
        const Json::Value *parent;
        std::string name;
        const Json::Value *value;
        
        AddValue() : parent(NULL), name(""), value(NULL) { }
        AddValue(const Json::Value *parent_, const std::string &name_, const Json::Value *value_) : parent(parent_), name(name_), value(value_) { }
    };
    
    struct ValidationContext {
        std::vector<Error> *errors;
        std::vector<AddValue> add_values;
        
        ValidationContext(std::vector<Error> *errors_) : errors(errors_) {
            errors->clear();
        }
        
        void add_error(const Error &error) {
            errors->push_back(error);
        }
        
        void add_value(const Json::Value &parent, const std::string &name, const Json::Value &value) {
            add_values.push_back(AddValue(&parent, name, &value));
        }
        
        size_t get_error_size() const { return errors->size(); }
        void truncate_errors(size_t size) { errors->resize(size); }

        size_t get_add_values_size() const { return add_values.size(); }
        void truncate_add_values(size_t size) { add_values.resize(size); }
        
        bool is_valid() const { return errors->empty(); }
    };
    
    static const std::string meta_schema;
    static Json::Value meta_schema_root;
    static SchemaValidator *meta_validator;
    
  typedef std::map<std::string, const Json::Value *> SchemaMap;
    
  explicit SchemaValidator(Json::Value schema, const Options &options, bool validate_schema);

  void init(const Options &options, bool validate_schema);

  // Each of the below methods handle a subset of the validation process. The
  // path paramater is the path to |instance| from the root of the instance tree
  // and is used in error messages.

  // Validates any instance node against any schema node. This is called for
  // every node in the instance tree, and it just decides which of the more
  // detailed methods to call.
  void Validate(const Json::Value &instance, const Json::Value &schema,
                const std::string& path, const ExpansionOptions &options, ValidationContext *context) const;

  // Validate, but does not keep errors
  bool isValid(const Json::Value &instance, const Json::Value &schema, const ExpansionOptions &options, ValidationContext *context) const;

  // Validates a node against a list of possible schemas. If any one of the
  // schemas match, the node is valid.
  bool ValidateChoices(const Json::Value &instance, const Json::Value &choices,
                       const std::string& path, ValidationContext *context) const;

  // Validates a node against a list of exact primitive values, eg 42, "foobar".
  void ValidateEnum(const Json::Value &instance, const Json::Value &choices,
                    const std::string& path, ValidationContext *context) const;

  // Validates a JSON object against an object schema node.
  void ValidateObject(const Json::Value &instance, const Json::Value &schema,
                      const std::string& path, const ExpansionOptions &options, ValidationContext *context) const;

  // Validates a JSON array against an array schema node.
  void ValidateArray(const Json::Value &instance, const Json::Value &schema,
                     const std::string& path, const ExpansionOptions &options, ValidationContext *context) const;

  // Validates a JSON array against an array schema node configured to be a
  // tuple. In a tuple, there is one schema node for each item expected in the
  // array.
  void ValidateTuple(const Json::Value &instance, const Json::Value &schema,
                     const std::string& path, ValidationContext *context) const;

  /// Validate a JSON string against a string schema node.
  void ValidateString(const Json::Value &instance, const Json::Value &schema,
                      const std::string& path, ValidationContext *context) const;

  /// Validate a JSON number against a number schema node.
  void ValidateNumber(const Json::Value &instance, const Json::Value &schema,
                      const std::string& path, ValidationContext *context) const;

  /// Validates that the JSON node |instance| conforms to |type|.
  bool ValidateType(const Json::Value &instance, const Json::Value& type,
                    const std::string& path, ValidationContext *context) const;

  /// Validates that the JSON node |instance| has |expected_type|.
  bool ValidateSimpleType(const Json::Value &instance, const std::string& expected_type,
                          const std::string& path, ValidationContext *context) const;

  /// Returns true if |schema| will allow additional items of any type.
  bool SchemaAllowsAnyAdditionalItems(
      const Json::Value &schema, Json::Value* addition_items_schema, const std::string& name) const;

  void collect_ids_refs(const Json::Value &node, URI base_uri, bool process_refs);

  const Json::Value *resolve_ref(const Json::Value *schema) const;
  std::string path_add(const std::string &path, const std::string &element) const;
  size_t count_utf8_characters(const std::string &str) const;

  // members that contain sub-schemata
  static const std::vector<std::string> schema_member_names;
  static const std::vector<std::string> schema_array_member_names;
  static const std::vector<std::string> schema_object_member_names;


  // The root for $ref expansion
  Json::Value refs_root_;

  // The root schema node.
  Json::Value *schema_root_;

  // resolved $refs
  std::unordered_map<const Json::Value *, const Json::Value *> refs;

  // only needed during initialization
  // map of $ids
  std::unordered_map<std::string, const Json::Value *> ids;
  std::unordered_set<const Json::Value *> sub_schemata;


  // Errors accumulated since the last call to Validate().
  std::vector<Error> errors_;


  /// \todo translate DISALLOW_COPY_AND_ASSIGN(SchemaValidator);
};

} // namespace nfotex_nsl

#endif  // CHROME_COMMON_JSON_SCHEMA_VALIDATOR_H_
