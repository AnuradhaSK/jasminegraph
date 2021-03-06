/**
Copyright 2018 JasmineGraph Team
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */


#include <iostream>
#include <unistd.h>
#include "main.h"

unsigned int microseconds = 10000000;
JasmineGraphServer* server;

void fnExit3 (void)
{
    delete(server);
    puts ("Shutting down the server.");
}


int main() {
    atexit(fnExit3);
    server = new JasmineGraphServer();
    server->run();

    while (server->isRunning())
    {
        usleep(microseconds);
    }

    delete server;
    return 0;
}

