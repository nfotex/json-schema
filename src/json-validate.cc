/*
    json-validate.cc -- validate a JSON document against a JSON Schema
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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <fstream>
#include <iostream>
#include <streambuf>

#include <json/json.h>
#include <json/SchemaValidator.h>

std::string read_file(const std::string &filename) {
    std::ifstream t(filename.c_str());
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
}

std::string read_stdin() {
    std::string str((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    return str;
}

[[noreturn]]
void usage(const char *prg, bool error) {
    FILE *f = error ? stderr : stdout;
    
    fprintf(f, "usage: %s [-h] [-D] [-p schema-pointer] schema [json]\n", prg);
    
    exit(error ? 1 : 0);
    
}

int main(int argc, char *argv[]) {
    std::string document;
    std::string pointer;
    auto add_defaults = false;

    int c;
    while ((c = getopt(argc, argv, "Dhp:")) != EOF) {
        switch (c) {
            case 'D':
                add_defaults = true;
                break;
                
            case 'p':
                pointer = optarg;
                break;
                
            case 'h':
                usage(argv[0], false);
                
            default:
                usage(argv[0], true);
        }
    }

    if (optind == argc) {
        usage(argv[0], true);
    }
    
    std::string schema_file = argv[optind++];
    std::string schema_str = read_file(schema_file);
    
    std::string document_file;
    if (optind != argc) {
        document_file = argv[optind];
        document = read_file(document_file);
    }
    else {
        document_file = "*stdin*";
        document = read_stdin();
    }
    
    std::string error_message;
    Json::SchemaValidator *validator = NULL;
    try {
        if (pointer.length() > 0) {
            validator = new Json::SchemaValidator(schema_str, pointer);
        }
        else {
            validator = new Json::SchemaValidator(schema_str);
        }
    }
    catch (Json::SchemaValidator::Exception e) {
        fprintf(stderr, "%s: can't create validator: %s\n", argv[0], e.type_message().c_str());
        for (std::vector<Json::SchemaValidator::Error>::const_iterator it = e.errors.begin(); it != e.errors.end(); ++it) {
            fprintf(stderr, "%s:%s: %s\n", document_file.c_str(), it->path.c_str(), it->message.c_str());
        }
        exit(1);
    }
    catch (std::exception e) {
        fprintf(stderr, "%s: can't create validator: %s\n", argv[0], e.what());
        exit(1);
    }
    
    Json::Reader reader;
    Json::Value root;
    
    bool success;
    
    success = reader.parse(document, root);

    if (!success) {
        fprintf(stderr, "%s", reader.getFormattedErrorMessages().c_str());
        exit(1);
    }

    std::vector<Json::SchemaValidator::Error> errors;
    bool ok;
    
    if (add_defaults) {
        ok = validator->validate_and_expand(root, true, &errors);
    }
    else {
        ok = validator->validate(root, &errors);
    }

    if (!ok) {
        for (std::vector<Json::SchemaValidator::Error>::const_iterator it = errors.begin(); it != errors.end(); ++it) {
            fprintf(stderr, "%s:%s: %s\n", document_file.c_str(), it->path.c_str(), it->message.c_str());
        }
        exit(1);
    }
    
    if (add_defaults) {
        Json::StyledWriter writer;
        
        printf("%s", writer.write(root).c_str());
    }
    
    exit(0);
}
