/*
    test-uri.cc -- test URI class
    Copyright 2018 nfotex IT DL GmbH.
 
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

#include <json/URI.h>

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: %s uri [reference]\n", argv[0]);
        exit(1);
    }

    Json::URI base(argv[1]);
    if (argc == 2) {
        if (base.has_scheme()) {
            printf("scheme: %s\n", base.get_scheme().c_str());
        }
        if (base.has_authority()) {
            printf("authority: %s\n", base.get_authority().c_str());
        }
        printf("path: %s\n", base.get_path().c_str());
        if (base.has_query()) {
            printf("query: %s\n", base.get_query().c_str());
        }
        if (base.has_fragment()) {
            printf("fragment: %s\n", base.get_fragment().c_str());
        }
    }
    else {
        Json::URI reference(argv[2]);
        auto resolved = base.resolve(reference);

        printf("%s\n", resolved.get_uri().c_str());
    }

    exit(0);
}
