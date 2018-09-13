/*
    json-pointer.cc -- access parts of a JSON document via JSON Pointer
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <istream>
#include <iterator>

#include <json/Pointer.h>

std::string read_stdin() {
    std::cin >> std::noskipws;

    // use stream iterators to copy the stream to a string
    std::istream_iterator<char> it(std::cin);
    std::istream_iterator<char> end;
    std::string results(it, end);

    return results;
}

void usage(const char *prg, bool error) {
    FILE *f = error ? stderr : stdout;

    fprintf(f, "usage: %s [-h] [-f input] [-o output] get path\n", prg);

    exit(error ? 1 : 0);
}

int main(int argc, char *argv[]) {
    // TODO implement -f/-o options
    // const char *infile = NULL;
    // const char *outfile = NULL;

    int c;
    while ((c = getopt(argc, argv, "f:ho:")) != EOF) {
        switch (c) {
            case 'f':
                // infile = optarg;
                break;

            case 'h':
                usage(argv[0], false);

            case 'o':
                // outfile = optarg;
                break;

            default:
                usage(argv[0], true);
        }
    }

    if (optind != argc - 2 || strcmp(argv[optind], "get") != 0) {
        usage(argv[0], false);
    }

    std::string input;

    input = read_stdin();

    Json::Pointer pointer;
    try {
        pointer = Json::Pointer(argv[optind+1]);
    }
    catch (std::exception &e) {
        fprintf(stderr, "%s: can't parse pointer '%s': %s\n", argv[0], argv[optind+1], e.what());
        exit(1);
    }

    Json::Value source;
    Json::Reader reader;
    reader.parse(input, source);
    Json::Value result;
    try {
        result = pointer.get(source);
    }
    catch (std::exception &e) {
        fprintf(stderr, "%s: can't apply pointer '%s': %s\n", argv[0], argv[optind+1], e.what());
        exit(1);
    }
    Json::StyledWriter writer;
    
    printf("%s", writer.write(result).c_str());
}

