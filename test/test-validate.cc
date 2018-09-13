/*
    test-validate.cc -- test SchemaValidator class
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <fstream>
#include <streambuf>

#include <json/json.h>
#include <json/SchemaValidator.h>

char *prg;

bool verbose = false;

static bool run_test(const Json::Value &test, unsigned int index);


std::string read_file(const std::string &filename) {
    std::ifstream t(filename.c_str());
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
}

[[noreturn]]
void usage(bool error) {
    FILE *f = error ? stderr : stdout;
    
    fprintf(f, "usage: %s [-hv] test\n", prg);
    
    exit(error ? 1 : 0);
    
}

int main(int argc, char *argv[]) {
    std::string document;
    
    prg = argv[0];
    
    int c;
    while ((c = getopt(argc, argv, "hv")) != EOF) {
        switch (c) {
            case 'h':
                usage(false);
                
            case 'v':
                verbose = true;
                break;
                
            default:
                usage(true);
        }
    }

    if (optind != argc -1) {
        usage(true);
    }
    
    std::string test_str = read_file(argv[optind]);
    
    if (test_str.length() == 0) {
        fprintf(stderr, "%s: can't read test case: %s\n", prg, strerror(errno));
        exit(1);
    }
    
    Json::Reader reader;
    Json::Value test_suite;
    
    if (!reader.parse(test_str, test_suite)) {
        fprintf(stderr, "%s: can't parse test case:\n", prg);
        fprintf(stderr, "%s", reader.getFormattedErrorMessages().c_str());
        exit(1);
    }
    
    unsigned int err = 0;
    for (Json::Value::ArrayIndex i = 0; i < test_suite.size(); i++) {
        if (!run_test(test_suite[i], i) ) {
            err++;
        }
    }
    
    exit(err == 0 ? 0 : 1);
}


static bool run_test(const Json::Value &test, unsigned int index) {
    std::string error_message;
    
    Json::SchemaValidator *validator = NULL;
    
    try {
        const Json::Value &schema = test["schema"];

        if (schema.isObject() && schema.isMember("$ref") && schema["$ref"].asString() == "http://json-schema.org/draft-07/schema#") {
            validator = Json::SchemaValidator::create_meta_validator();
        }
        else {
            validator = new Json::SchemaValidator(schema);
        }
    }
    catch (Json::SchemaValidator::Exception e) {
        fprintf(stderr, "%s: %u: can't create validator: %s\n", prg, index, e.type_message().c_str());
        for (std::vector<Json::SchemaValidator::Error>::const_iterator it = e.errors.begin(); it != e.errors.end(); ++it) {
            fprintf(stderr, "%s: %s\n", it->path.c_str(), it->message.c_str());
        }
        return false;
    }

    const Json::Value &tests = test["tests"];
    
    unsigned int err = 0;
    
    for (Json::Value::ArrayIndex i = 0; i < tests.size(); i++) {
        const Json::Value &test_case = tests[i];
        bool valid = validator->validate(test_case["data"]);
        
        if (valid != test_case["valid"].asBool()) {
            err++;
            if (verbose) {
                printf("%u.%u %s / %s - expected: %s, got: %s\n", index, i, test["description"].asCString(), test_case["description"].asCString(), valid ? "invalid" : "valid", valid ? "valid" : "invalid");
                if (!valid) {
                    const std::vector<Json::SchemaValidator::Error> errors = validator->errors();
                    
                    for (std::vector<Json::SchemaValidator::Error>::const_iterator it = errors.begin(); it != errors.end(); ++it) {
                        fprintf(stderr, "    %s: %s\n", it->path.c_str(), it->message.c_str());
                    }
                }
            }
        }
    }

    if (verbose && 0) { // disable for now
        printf("%u: %d tests, %u ok, %u failed\n", index, tests.size(), tests.size() - err, err);
    }
    
    return (err == 0);
}
