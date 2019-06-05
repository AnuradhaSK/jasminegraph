/**
Copyright 2018 JasminGraph Team
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

#include <cstring>
#include "JasmineGraphInstanceService.h"
#include "../util/Utils.h"
#include "../util/logger/Logger.h"
#include "../trainer/python-c-api/Python_C_API.h"
#include "JasmineGraphInstance.h"

using namespace std;
Logger instance_logger;
pthread_mutex_t file_lock;
StatisticCollector collector;


char *converter(const std::string &s) {
    char *pc = new char[s.size() + 1];
    std::strcpy(pc, s.c_str());
    return pc;
}

void *instanceservicesession(void *dummyPt) {
    instanceservicesessionargs *sessionargs = (instanceservicesessionargs *) dummyPt;
    int connFd = sessionargs->connFd;
    std::map<std::string,JasmineGraphHashMapLocalStore> graphDBMapLocalStores = sessionargs->graphDBMapLocalStores;
    std::map<std::string,JasmineGraphHashMapCentralStore> graphDBMapCentralStores = sessionargs->graphDBMapCentralStores;

    string serverName = sessionargs->hostName;
    int serverPort = sessionargs->port;
    int serverDataPort = sessionargs->dataPort;


    instance_logger.log("New service session started", "info");
    Utils utils;
    collector.init();

    utils.createDirectory(utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder"));

    char data[300];
    bool loop = false;
    while (!loop) {
        bzero(data, 301);
        read(connFd, data, 300);

        string line = (data);

        Utils utils;
        line = utils.trim_copy(line, " \f\n\r\t\v");

        if (line.compare(JasmineGraphInstanceProtocol::HANDSHAKE) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::HANDSHAKE, "info");
            write(connFd, JasmineGraphInstanceProtocol::HANDSHAKE_OK.c_str(),
                  JasmineGraphInstanceProtocol::HANDSHAKE_OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::HANDSHAKE_OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);
            line = utils.trim_copy(line, " \f\n\r\t\v");
            string server_hostname = line;
            instance_logger.log("Received hostname : " + line, "info");
            std::cout << "ServerName : " << server_hostname << std::endl;
        } else if (line.compare(JasmineGraphInstanceProtocol::CLOSE) == 0) {
            write(connFd, JasmineGraphInstanceProtocol::CLOSE_ACK.c_str(),
                  JasmineGraphInstanceProtocol::CLOSE_ACK.size());
            close(connFd);
        } else if (line.compare(JasmineGraphInstanceProtocol::SHUTDOWN) == 0) {
            write(connFd, JasmineGraphInstanceProtocol::SHUTDOWN_ACK.c_str(),
                  JasmineGraphInstanceProtocol::SHUTDOWN_ACK.size());
            close(connFd);
            break;
        } else if (line.compare(JasmineGraphInstanceProtocol::READY) == 0) {
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
        }
        else if (line.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_NAME.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_NAME.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_NAME, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string fileName = (data);
            instance_logger.log("Received File name: " + fileName, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_LEN.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_LEN.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string size = (data);
            //int fileSize = atoi(size.c_str());
            instance_logger.log("Received file size in bytes: " + size, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_CONT.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_CONT.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
            string fullFilePath =
                    utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + fileName;
            int fileSize = atoi(size.c_str());
            while (true){
                if (utils.fileExists(fullFilePath)){
                    while (utils.getFileSize(fullFilePath) < fileSize) {
                        bzero(data, 301);
                        read(connFd, data, 300);
                        line = (data);
                        if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                            write(connFd, JasmineGraphInstanceProtocol::FILE_RECV_WAIT.c_str(),
                                  JasmineGraphInstanceProtocol::FILE_RECV_WAIT.size());
                        }
                    }
                    break;
                }else{
                    sleep(1);
                    continue;
                }

            }
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);

            if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::FILE_ACK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
            }

            instance_logger.log("File received and saved to " + fullFilePath, "info");
            loop = true;

            utils.unzipFile(fullFilePath);
            size_t lastindex = fileName.find_last_of(".");
            string rawname = fileName.substr(0, lastindex);
            fullFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + rawname;

            string partitionID = rawname.substr(rawname.find_last_of("_") + 1);
            pthread_mutex_lock(&file_lock);
            writeCatalogRecord(graphID +":"+partitionID);
            pthread_mutex_unlock(&file_lock);

            while (!utils.fileExists(fullFilePath)) {
                bzero(data, 301);
                read(connFd, data, 300);
                string response = (data);
                response = utils.trim_copy(response, " \f\n\r\t\v");
                if (response.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                    write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.c_str(),
                          JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.size());
                    instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT, "info");
                }
            }
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);
            if (line.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.c_str(),
                      JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK, "info");
            }

        } else if (line.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CENTRAL) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CENTRAL, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_NAME.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_NAME.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_NAME, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string fileName = (data);
            //fileName = utils.trim_copy(fileName, " \f\n\r\t\v");
            instance_logger.log("Received File name: " + fileName, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_LEN.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_LEN.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string size = (data);
            instance_logger.log("Received file size in bytes: " + size, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_CONT.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_CONT.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
            string fullFilePath =
                    utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + fileName;
            int fileSize = atoi(size.c_str());
            while (true){
                if (utils.fileExists(fullFilePath)){
                    while (utils.getFileSize(fullFilePath) < fileSize) {
                        bzero(data, 301);
                        read(connFd, data, 300);
                        line = (data);

                        if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                            write(connFd, JasmineGraphInstanceProtocol::FILE_RECV_WAIT.c_str(),
                                  JasmineGraphInstanceProtocol::FILE_RECV_WAIT.size());
                        }
                    }
                    break;
                }else{
                    sleep(1);
                    continue;
                }

            }

            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);

            if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::FILE_ACK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
            }

            instance_logger.log("File received and saved to " + fullFilePath, "info");
            loop = true;

            utils.unzipFile(fullFilePath);
            size_t lastindex = fileName.find_last_of(".");
            string rawname = fileName.substr(0, lastindex);
            fullFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + rawname;

            while (!utils.fileExists(fullFilePath)) {
                bzero(data, 301);
                read(connFd, data, 300);
                string response = (data);
                response = utils.trim_copy(response, " \f\n\r\t\v");
                if (response.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                    write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.c_str(),
                          JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.size());
                    instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT, "info");
                }
            }
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);
            if (line.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.c_str(),
                      JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK, "info");
            }
        } else if (line.compare(JasmineGraphInstanceProtocol::UPLOAD_RDF_ATTRIBUTES) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::UPLOAD_RDF_ATTRIBUTES, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_NAME.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_NAME.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_NAME, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string fileName = (data);
            //fileName = utils.trim_copy(fileName, " \f\n\r\t\v");
            instance_logger.log("Received File name: " + fileName, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_LEN.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_LEN.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string size = (data);
            instance_logger.log("Received file size in bytes: " + size, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_CONT.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_CONT.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
            string fullFilePath =
                    utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + fileName;
            int fileSize = atoi(size.c_str());
            while (true){
                if (utils.fileExists(fullFilePath)){
                    while (utils.getFileSize(fullFilePath) < fileSize) {
                        bzero(data, 301);
                        read(connFd, data, 300);
                        line = (data);
                        if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                            write(connFd, JasmineGraphInstanceProtocol::FILE_RECV_WAIT.c_str(),
                                  JasmineGraphInstanceProtocol::FILE_RECV_WAIT.size());
                        }
                    }
                    break;
                }else{
                    sleep(1);
                    continue;
                }

            }

            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);

            if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::FILE_ACK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
            }

            instance_logger.log("File received and saved to " + fullFilePath, "info");
            loop = true;

            utils.unzipFile(fullFilePath);
            size_t lastindex = fileName.find_last_of(".");
            string rawname = fileName.substr(0, lastindex);
            fullFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + rawname;

            while (!utils.fileExists(fullFilePath)) {
                bzero(data, 301);
                read(connFd, data, 300);
                string response = (data);
                response = utils.trim_copy(response, " \f\n\r\t\v");
                if (response.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                    write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.c_str(),
                          JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.size());
                    instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT, "info");
                }
            }
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);
            if (line.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.c_str(),
                      JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK, "info");
            }
        }else if (line.compare(JasmineGraphInstanceProtocol::UPLOAD_RDF_ATTRIBUTES_CENTRAL) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::UPLOAD_RDF_ATTRIBUTES_CENTRAL, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_NAME.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_NAME.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_NAME, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string fileName = (data);
            //fileName = utils.trim_copy(fileName, " \f\n\r\t\v");
            instance_logger.log("Received File name: " + fileName, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_LEN.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_LEN.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string size = (data);
            instance_logger.log("Received file size in bytes: " + size, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_CONT.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_CONT.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
            string fullFilePath =
                    utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + fileName;
            int fileSize = atoi(size.c_str());
            while (true){
                if (utils.fileExists(fullFilePath)){
                    while (utils.getFileSize(fullFilePath) < fileSize) {
                        bzero(data, 301);
                        read(connFd, data, 300);
                        line = (data);
                        if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                            write(connFd, JasmineGraphInstanceProtocol::FILE_RECV_WAIT.c_str(),
                                  JasmineGraphInstanceProtocol::FILE_RECV_WAIT.size());
                        }
                    }
                    break;
                }else{
                    sleep(1);
                    continue;
                }

            }

            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);

            if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::FILE_ACK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
            }

            instance_logger.log("File received and saved to " + fullFilePath, "info");
            loop = true;

            utils.unzipFile(fullFilePath);
            size_t lastindex = fileName.find_last_of(".");
            string rawname = fileName.substr(0, lastindex);
            fullFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + rawname;

            while (!utils.fileExists(fullFilePath)) {
                bzero(data, 301);
                read(connFd, data, 300);
                string response = (data);
                response = utils.trim_copy(response, " \f\n\r\t\v");
                if (response.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                    write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.c_str(),
                          JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT.size());
                    instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_WAIT, "info");
                }
            }
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);
            if (line.compare(JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.c_str(),
                      JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::BATCH_UPLOAD_ACK, "info");
            }
        } else if (line.compare(JasmineGraphInstanceProtocol::DELETE_GRAPH) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::DELETE_GRAPH, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_PARTITION_ID.c_str(),
                  JasmineGraphInstanceProtocol::SEND_PARTITION_ID.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_PARTITION_ID, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string partitionID = (data);
            instance_logger.log("Received partition ID: " + partitionID, "info");
            deleteGraphPartition(graphID,partitionID);
            //pthread_mutex_lock(&file_lock);
            //TODO :: Update catalog file
            //pthread_mutex_unlock(&file_lock);
            string result = "1";
            write(connFd, result.c_str(), result.size());
            instance_logger.log("Sent : " + result, "info");
        } else if (line.compare(JasmineGraphInstanceProtocol::TRIANGLES) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::TRIANGLES, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");

            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            bzero(data, 301);
            read(connFd, data, 300);
            string partitionId = (data);
            partitionId = utils.trim_copy(partitionId, " \f\n\r\t\v");
            instance_logger.log("Received Partition ID: " + partitionId, "info");
            long localCount = countLocalTriangles(graphID,partitionId,graphDBMapLocalStores,graphDBMapCentralStores);
            std::string result = to_string(localCount);
            write(connFd, result.c_str(), result.size());
        } else if (line.compare(JasmineGraphInstanceProtocol::SEND_CENTRALSTORE_TO_AGGREGATOR) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::SEND_CENTRALSTORE_TO_AGGREGATOR, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");

            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            bzero(data, 301);
            read(connFd, data, 300);
            string partitionId = (data);
            partitionId = utils.trim_copy(partitionId, " \f\n\r\t\v");
            instance_logger.log("Received Partition ID: " + partitionId, "info");

            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            bzero(data, 301);
            read(connFd, data, 300);
            string aggregatorHost = (data);
            aggregatorHost = utils.trim_copy(aggregatorHost, " \f\n\r\t\v");
            instance_logger.log("Received Aggregator Host: " + aggregatorHost, "info");

            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            bzero(data, 301);
            read(connFd, data, 300);
            string aggregatorPort = (data);
            aggregatorPort = utils.trim_copy(aggregatorPort, " \f\n\r\t\v");
            instance_logger.log("Received Aggregator Port: " + aggregatorPort, "info");

            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            bzero(data, 301);
            read(connFd, data, 300);
            string host = (data);
            host = utils.trim_copy(host, " \f\n\r\t\v");
            instance_logger.log("Received Host: " + host, "info");

            std::string result = JasmineGraphInstanceService::copyCentralStoreToAggregator(graphID,partitionId,aggregatorHost,aggregatorPort,host);

            write(connFd, result.c_str(), result.size());
        } else if (line.compare(JasmineGraphInstanceProtocol::AGGREGATE_CENTRALSTORE_TRIANGLES) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::AGGREGATE_CENTRALSTORE_TRIANGLES, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphId = (data);
            graphId = utils.trim_copy(graphId, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphId, "info");

            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            bzero(data, 301);
            read(connFd, data, 300);
            string partitionId = (data);
            partitionId = utils.trim_copy(partitionId, " \f\n\r\t\v");
            instance_logger.log("Received Partition ID: " + partitionId, "info");
            long aggregatedTriangleCount =0;

            aggregatedTriangleCount= JasmineGraphInstanceService::aggregateCentralStoreTriangles(graphId, partitionId);
            write(connFd, std::to_string(aggregatedTriangleCount).c_str(), std::to_string(aggregatedTriangleCount).size());
        } else if (line.compare(JasmineGraphInstanceProtocol::PERFORMANCE_STATISTICS) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::PERFORMANCE_STATISTICS, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string isVMStatManager = (data);
            isVMStatManager = utils.trim_copy(isVMStatManager, " \f\n\r\t\v");
            instance_logger.log("Received VM Stat manager status: " + isVMStatManager, "info");

            std::string memoryUsage = JasmineGraphInstanceService::requestPerformanceStatistics(isVMStatManager);
            write(connFd, memoryUsage.c_str(), memoryUsage.size());
            instance_logger.log("Done and dusted", "info");
        } else if (line.compare(JasmineGraphInstanceProtocol::INITIATE_TRAIN) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::INITIATE_TRAIN, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string trainData(data);

            Utils utils;
            trainData = utils.trim_copy(trainData, " \f\n\r\t\v");

            std::vector<std::string> trainargs = Utils::split(trainData, ' ');

            std::vector<char *> vc;
            std::transform(trainargs.begin(), trainargs.end(), std::back_inserter(vc), converter);

            Python_C_API::train(vc.size(), &vc[0]);
            loop = true;

        } else if (line.compare(JasmineGraphInstanceProtocol::INITIATE_PREDICT) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::INITIATE_PREDICT, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");

            /*Receive hosts' detail*/
            write(connFd, JasmineGraphInstanceProtocol::SEND_HOSTS.c_str(),
                  JasmineGraphInstanceProtocol::SEND_HOSTS.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_HOSTS, "info");

            char dataBuffer[1024];
            bzero(dataBuffer,1025);
            read(connFd, dataBuffer, 1025);
            string hostList = (dataBuffer);
            instance_logger.log("Received Hosts List: " + hostList, "info");

            //Put all hosts to a map
            std::map<std::string, JasmineGraphInstanceService::workerPartitions> graphPartitionedHosts;
            std::vector<std::string> hosts = Utils::split(hostList, '|');
            int count = 0;
            int totalPartitions = 0;
            for (std::vector<std::string>::iterator it = hosts.begin();
                 it != hosts.end(); ++it) {
                if(count != 0){
                    std::vector<std::string> hostDetail = Utils::split(*it, ',');
                    std::string hostName;
                    int port;
                    int dataport;
                    std::vector<string> partitionIDs;
                    for (std::vector<std::string>::iterator j = hostDetail.begin();
                         j != hostDetail.end(); ++j) {
                        int index = std::distance(hostDetail.begin(), j);
                        if (index == 0) {
                            hostName = *j;
                        } else if (index == 1) {
                            port = stoi(*j);
                        } else if (index == 2) {
                            dataport = stoi(*j);
                        } else {
                            partitionIDs.push_back(*j);
                            totalPartitions +=1;
                        }
                    }
                    graphPartitionedHosts.insert(
                            pair<string, JasmineGraphInstanceService::workerPartitions>(hostName,
                                                                               {port, dataport, partitionIDs}));
                }
                count++;
            }
            /*Receive file*/
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_NAME.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_NAME.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_NAME, "info");

            bzero(data, 301);
            read(connFd, data, 300);
            string fileName = (data);
            instance_logger.log("Received File name: " + fileName, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_LEN.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_LEN.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
            bzero(data, 301);
            read(connFd, data, 300);
            string size = (data);
            instance_logger.log("Received file size in bytes: " + size, "info");
            write(connFd, JasmineGraphInstanceProtocol::SEND_FILE_CONT.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_CONT.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
            string fullFilePath =
                    utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + fileName;
            int fileSize = atoi(size.c_str());
            while (utils.fileExists(fullFilePath) && utils.getFileSize(fullFilePath) < fileSize) {
                bzero(data, 301);
                read(connFd, data, 300);
                line = (data);

                if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                    write(connFd, JasmineGraphInstanceProtocol::FILE_RECV_WAIT.c_str(),
                          JasmineGraphInstanceProtocol::FILE_RECV_WAIT.size());
                }
            }

            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);

            if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                write(connFd, JasmineGraphInstanceProtocol::FILE_ACK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
            }
            JasmineGraphInstanceService::collectTrainedModels(sessionargs,graphID,graphPartitionedHosts,totalPartitions);
            loop = true;
        } else if(line.compare(JasmineGraphInstanceProtocol::INITIATE_MODEL_COLLECTION) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::INITIATE_MODEL_COLLECTION, "info");
            write(connFd, JasmineGraphInstanceProtocol::OK.c_str(), JasmineGraphInstanceProtocol::OK.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::OK, "info");

            bzero(data, 301);
            read(connFd, data, 300);
            string serverHostName = (data);
            serverHostName = utils.trim_copy(serverHostName, " \f\n\r\t\v");
            instance_logger.log("Received Port: " + serverHostName, "info");

            bzero(data, 301);
            read(connFd, data, 300);
            string serverHostPort = (data);
            serverHostPort = utils.trim_copy(serverHostPort, " \f\n\r\t\v");
            instance_logger.log("Received Port: " + serverHostPort, "info");

            bzero(data, 301);
            read(connFd, data, 300);
            string serverHostDataPort = (data);
            serverHostDataPort = utils.trim_copy(serverHostDataPort, " \f\n\r\t\v");
            instance_logger.log("Received Data Port: " + serverHostDataPort, "info");

            bzero(data, 301);
            read(connFd, data, 300);
            string graphID = (data);
            graphID = utils.trim_copy(graphID, " \f\n\r\t\v");
            instance_logger.log("Received Graph ID: " + graphID, "info");

            bzero(data, 301);
            read(connFd, data, 300);
            string partitionID = (data);
            partitionID = utils.trim_copy(partitionID, " \f\n\r\t\v");
            instance_logger.log("Received Partition ID: " + partitionID, "info");
            //zip the file
//          TODO::update file name/path accordingly after zipping

            std::string fileName = graphID+"_"+partitionID+"_model";
            std::string filePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.trainedmodelfolder") + "/" +fileName;
            int fileSize = utils.getFileSize(filePath);
            std::string fileLength = to_string(fileSize);
            //send file name
            bzero(data, 301);
            read(connFd, data, 300);
            line = (data);
            if(line.compare(JasmineGraphInstanceProtocol::SEND_FILE_NAME)==0){
                write(connFd, fileName.c_str(), fileName.size());
                instance_logger.log("Sent : File name " + fileName, "info");
                bzero(data, 301);
                read(connFd, data, 300);
                line = (data);
                //send file length
                if(line.compare(JasmineGraphInstanceProtocol::SEND_FILE_LEN)==0){
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
                    write(connFd, fileLength.c_str(), fileLength.size());
                    instance_logger.log("Sent : File length in bytes " + fileLength, "info");
                    bzero(data, 301);
                    read(connFd, data, 300);
                    line = (data);
                    //send content
                    if (line.compare(JasmineGraphInstanceProtocol::SEND_FILE_CONT) == 0) {
                        instance_logger.log("Received : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
                        instance_logger.log("Going to send file through service", "info");
                        JasmineGraphInstance::sendFileThroughService(serverHostName, stoi(serverHostDataPort), fileName, filePath);
                    }
                }
            }
            int count = 0;
            while (true) {
                write(connFd, JasmineGraphInstanceProtocol::FILE_RECV_CHK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_RECV_CHK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                instance_logger.log("Checking if file is received", "info");
                bzero(data, 301);
                read(connFd, data, 300);
                line = (data);
                //response = utils.trim_copy(response, " \f\n\r\t\v");

                if (line.compare(JasmineGraphInstanceProtocol::FILE_RECV_WAIT) == 0) {
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_WAIT, "info");
                    instance_logger.log("Checking file status : " + to_string(count), "info");
                    count++;
                    sleep(1);
                    continue;
                } else if (line.compare(JasmineGraphInstanceProtocol::FILE_ACK) == 0) {
                    instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
                    instance_logger.log("File transfer completed", "info");
                    break;
                }
            }
        }
    }
    instance_logger.log("Closing thread " + to_string(pthread_self()), "info");
    close(connFd);
}

JasmineGraphInstanceService::JasmineGraphInstanceService() {

}

int JasmineGraphInstanceService::run(string hostName,int serverPort, int serverDataPort) {

    int listenFd;
    socklen_t len;
    struct sockaddr_in svrAdd;
    struct sockaddr_in clntAdd;

    //create socket
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::cerr << "Cannot open socket" << std::endl;
        return 0;
    }

    bzero((char *) &svrAdd, sizeof(svrAdd));

    svrAdd.sin_family = AF_INET;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
    svrAdd.sin_port = htons(serverPort);

    int yes = 1;

    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }


    //bind socket
    if (bind(listenFd, (struct sockaddr *) &svrAdd, sizeof(svrAdd)) < 0) {
        std::cerr << "Cannot bind" << std::endl;
        return 0;
    }

    listen(listenFd, 10);

    len = sizeof(clntAdd);

    int connectionCounter = 0;
    pthread_mutex_init(&file_lock, NULL);
    pthread_t threadA[300];

    // TODO :: What is the maximum number of connections allowed??
    while (connectionCounter < 300) {
        instance_logger.log("Worker listening on port " + to_string(serverPort), "info");
        int connFd = accept(listenFd, (struct sockaddr *) &clntAdd, &len);
        std::map<std::string,JasmineGraphHashMapLocalStore> graphDBMapLocalStores;
        std::map<std::string,JasmineGraphHashMapCentralStore> graphDBMapCentralStores;

        if (connFd < 0) {
            instance_logger.log("Cannot accept connection to port " + to_string(serverPort), "error");
        } else {
            instance_logger.log("Connection successful to port " + to_string(serverPort), "info");
            struct instanceservicesessionargs instanceservicesessionargs1;
            instanceservicesessionargs1.connFd = connFd;
            instanceservicesessionargs1.graphDBMapLocalStores = graphDBMapLocalStores;
            instanceservicesessionargs1.graphDBMapCentralStores = graphDBMapCentralStores;
            instanceservicesessionargs1.port = serverPort;
            instanceservicesessionargs1.hostName = hostName;
            instanceservicesessionargs1.dataPort = serverDataPort;


            pthread_create(&threadA[connectionCounter], NULL, instanceservicesession,
                           &instanceservicesessionargs1);
            //pthread_detach(threadA[connectionCounter]);
            //pthread_join(threadA[connectionCounter], NULL);
            connectionCounter++;
        }
    }

    for (int i = 0; i < connectionCounter; i++) {
        pthread_join(threadA[i], NULL);
        std::cout << "service Threads joined" << std::endl;
    }

    pthread_mutex_destroy(&file_lock);
}

void deleteGraphPartition(std::string graphID, std::string partitionID) {
    Utils utils;
    string partitionFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + graphID + +"_"+ partitionID;
    utils.deleteDirectory(partitionFilePath);
    string centalStoreFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + graphID + +"_centralstore_"+ partitionID;
    utils.deleteDirectory(centalStoreFilePath);
    string attributeFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + graphID + +"_attributes_"+ partitionID;
    utils.deleteDirectory(attributeFilePath);
    string attributeCentalStoreFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + graphID + +"_centralstore_attributes_"+ partitionID;
    utils.deleteDirectory(attributeCentalStoreFilePath);
    instance_logger.log("Graph partition and centralstore files are now deleted", "info");
}

void writeCatalogRecord(string record) {
    Utils utils;
    utils.createDirectory(utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder"));
    string catalogFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder")+"/catalog.txt";
    ofstream outfile;
    outfile.open(catalogFilePath.c_str(), std::ios_base::app);
    outfile << record << endl;
    outfile.close();
}



long countLocalTriangles(std::string graphId, std::string partitionId, std::map<std::string,JasmineGraphHashMapLocalStore> graphDBMapLocalStores, std::map<std::string,JasmineGraphHashMapCentralStore> graphDBMapCentralStores) {
    long result;


    std::string graphIdentifier = graphId + "_" + partitionId;
    std::string centralGraphIdentifier = graphId + +"_centralstore_"+ partitionId;
    JasmineGraphHashMapLocalStore graphDB;
    JasmineGraphHashMapCentralStore centralGraphDB;

    std::map<std::string,JasmineGraphHashMapLocalStore>::iterator localMapIterator = graphDBMapLocalStores.find(graphIdentifier);
    std::map<std::string,JasmineGraphHashMapCentralStore>::iterator centralStoreIterator = graphDBMapCentralStores.find(graphIdentifier);


    if (localMapIterator == graphDBMapLocalStores.end()) {
        if (JasmineGraphInstanceService::isGraphDBExists(graphId,partitionId)) {
            JasmineGraphInstanceService::loadLocalStore(graphId,partitionId,graphDBMapLocalStores);
        }
        graphDB = graphDBMapLocalStores[graphIdentifier];
    } else {
        graphDB = graphDBMapLocalStores[graphIdentifier];
    }

    if (centralStoreIterator == graphDBMapCentralStores.end()) {
        if (JasmineGraphInstanceService::isInstanceCentralStoreExists(graphId,partitionId)) {
            JasmineGraphInstanceService::loadInstanceCentralStore(graphId,partitionId,graphDBMapCentralStores);
        }
        centralGraphDB = graphDBMapCentralStores[centralGraphIdentifier];
    } else {
        centralGraphDB = graphDBMapCentralStores[centralGraphIdentifier];
    }

    result = Triangles::run(graphDB,centralGraphDB,graphId,partitionId);

    return result;

}

bool JasmineGraphInstanceService::isGraphDBExists(std::string graphId, std::string partitionId) {
    Utils utils;
    std::string dataFolder = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder");
    std::string fileName = dataFolder + "/" + graphId + "_"+partitionId;
    std::ifstream dbFile(fileName, std::ios::binary);
    if (!dbFile) {
        return false;
    }
    return true;
}

bool JasmineGraphInstanceService::isInstanceCentralStoreExists(std::string graphId, std::string partitionId) {
    Utils utils;
    std::string dataFolder = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder");
    std::string filename = dataFolder+"/"+graphId + +"_centralstore_"+ partitionId;
    std::ifstream dbFile(filename, std::ios::binary);
    if (!dbFile) {
        return false;
    }
    return true;
}

void JasmineGraphInstanceService::loadLocalStore(std::string graphId, std::string partitionId, std::map<std::string,JasmineGraphHashMapLocalStore>& graphDBMapLocalStores) {
    std::string graphIdentifier = graphId + "_"+partitionId;
    Utils utils;
    std::string folderLocation = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder");
    JasmineGraphHashMapLocalStore  *jasmineGraphHashMapLocalStore = new JasmineGraphHashMapLocalStore(atoi(graphId.c_str()),atoi(partitionId.c_str()), folderLocation);
    jasmineGraphHashMapLocalStore->loadGraph();
    graphDBMapLocalStores.insert(std::make_pair(graphIdentifier,*jasmineGraphHashMapLocalStore));
}

void JasmineGraphInstanceService::loadInstanceCentralStore(std::string graphId, std::string partitionId,
                                                           std::map<std::string, JasmineGraphHashMapCentralStore>& graphDBMapCentralStores) {
    std::string graphIdentifier = graphId + +"_centralstore_"+ partitionId;
    Utils utils;
    JasmineGraphHashMapCentralStore *jasmineGraphHashMapCentralStore = new JasmineGraphHashMapCentralStore(atoi(graphId.c_str()),atoi(partitionId.c_str()));
    jasmineGraphHashMapCentralStore->loadGraph();
    graphDBMapCentralStores.insert(std::make_pair(graphIdentifier,*jasmineGraphHashMapCentralStore));
}

JasmineGraphHashMapCentralStore JasmineGraphInstanceService::loadCentralStore(std::string centralStoreFileName) {
    JasmineGraphHashMapCentralStore *jasmineGraphHashMapCentralStore = new JasmineGraphHashMapCentralStore();
    jasmineGraphHashMapCentralStore->loadGraph(centralStoreFileName);
    return *jasmineGraphHashMapCentralStore;
}

std::string JasmineGraphInstanceService::copyCentralStoreToAggregator(std::string graphId, std::string partitionId,
                                                                      std::string aggregatorHost,
                                                                      std::string aggregatorPort, std::string host) {
    Utils utils;
    char buffer[128];
    std::string result = "SUCCESS";
    std::string centralGraphIdentifier = graphId + +"_centralstore_"+ partitionId;
    std::string dataFolder = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder");
    std::string aggregatorFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.aggregatefolder");

    if (JasmineGraphInstanceService::isInstanceCentralStoreExists(graphId,partitionId)) {
        std::string centralStoreFile = dataFolder + "/" + centralGraphIdentifier;
        std::string copyCommand;

        DIR* dir = opendir(aggregatorFilePath.c_str());

        if (dir) {
            closedir(dir);
        } else {
            std::string createDirCommand = "mkdir -p " + aggregatorFilePath;
            FILE *createDirInput = popen(createDirCommand.c_str(),"r");
            pclose(createDirInput);
        }

        if (aggregatorHost == host) {
            copyCommand = "cp "+centralStoreFile+ " " + aggregatorFilePath;
        } else {
            copyCommand = "scp "+centralStoreFile+" "+ aggregatorHost+":"+aggregatorFilePath;
        }

        FILE *copyInput = popen(copyCommand.c_str(),"r");

        if (copyInput) {
            // read the input
            while (!feof(copyInput)) {
                if (fgets(buffer, 128, copyInput) != NULL) {
                    result.append(buffer);
                }
            }
            if (!result.empty()) {
                std::cout<<result<< std::endl;
            }
            pclose(copyInput);
        }


    }

    return result;

}

long JasmineGraphInstanceService::aggregateCentralStoreTriangles(std::string graphId, std::string partitionId) {
    Utils utils;
    std::string aggregatorFilePath = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.aggregatefolder");
    std::vector<std::string> fileNames;
    map<long, unordered_set<long>> aggregatedCentralStore;
    std::string centralGraphIdentifier = graphId + +"_centralstore_"+ partitionId;
    std::string dataFolder = utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder");
    std::string workerCentralStoreFile = dataFolder + "/" + centralGraphIdentifier;
    JasmineGraphHashMapCentralStore workerCentralStore = JasmineGraphInstanceService::loadCentralStore(workerCentralStoreFile);
    map<long, unordered_set<long>> workerCentralGraphMap = workerCentralStore.getUnderlyingHashMap();

    map<long, unordered_set<long>>::iterator workerCentalGraphIterator;

    for (workerCentalGraphIterator = workerCentralGraphMap.begin(); workerCentalGraphIterator != workerCentralGraphMap.end();++workerCentalGraphIterator) {
        long startVid = workerCentalGraphIterator->first;
        unordered_set<long> endVidSet = workerCentalGraphIterator->second;

        unordered_set<long> aggregatedEndVidSet = aggregatedCentralStore[startVid];
        aggregatedEndVidSet.insert(endVidSet.begin(),endVidSet.end());
        aggregatedCentralStore[startVid] = aggregatedEndVidSet;
    }


    DIR* dirp = opendir(aggregatorFilePath.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
        fileNames.push_back(dp->d_name);
    }
    closedir(dirp);

    std::vector<std::string>::iterator fileNamesIterator;

    for (fileNamesIterator = fileNames.begin(); fileNamesIterator != fileNames.end(); ++fileNamesIterator) {
        std::string fileName = *fileNamesIterator;
        struct stat s;
        std::string centralStoreFile = aggregatorFilePath + "/" + fileName;
        if (stat(centralStoreFile.c_str(),&s) == 0) {
            if (s.st_mode & S_IFREG) {
                JasmineGraphHashMapCentralStore centralStore = JasmineGraphInstanceService::loadCentralStore(centralStoreFile);
                map<long, unordered_set<long>> centralGraphMap = centralStore.getUnderlyingHashMap();
                map<long, unordered_set<long>>::iterator centralGraphMapIterator;

                for (centralGraphMapIterator = centralGraphMap.begin(); centralGraphMapIterator != centralGraphMap.end(); ++centralGraphMapIterator) {
                    long startVid = centralGraphMapIterator->first;
                    unordered_set<long> endVidSet = centralGraphMapIterator->second;

                    unordered_set<long> aggregatedEndVidSet = aggregatedCentralStore[startVid];
                    aggregatedEndVidSet.insert(endVidSet.begin(),endVidSet.end());
                    aggregatedCentralStore[startVid] = aggregatedEndVidSet;
                }
            }
        }

    }
    map<long, long> distributionHashMap = JasmineGraphInstanceService::getOutDegreeDistributionHashMap(aggregatedCentralStore);

    long aggregatedTriangleCount = Triangles::countCentralStoreTriangles(aggregatedCentralStore,distributionHashMap);

    return aggregatedTriangleCount;

}

map<long, long> JasmineGraphInstanceService::getOutDegreeDistributionHashMap(map<long, unordered_set<long>> graphMap) {
    map<long, long> distributionHashMap;

    for (map<long, unordered_set<long>>::iterator it = graphMap.begin(); it != graphMap.end(); ++it) {
        long distribution = (it->second).size();
        distributionHashMap.insert(std::make_pair(it->first, distribution));
    }
    return distributionHashMap;
}

std::string JasmineGraphInstanceService::requestPerformanceStatistics(std::string isVMStatManager) {
    Utils utils;
    int memoryUsage = collector.getVirtualMemoryUsage();
    double cpuUsage = collector.getCpuUsage();
    auto executedTime = std::chrono::system_clock::now();
    std::time_t reportTime = std::chrono::system_clock::to_time_t(executedTime);
    std::string reportTimeString(std::ctime(&reportTime));
    reportTimeString = utils.trim_copy(reportTimeString, " \f\n\r\t\v");
    std::string usageString = reportTimeString+","+to_string(memoryUsage)+","+to_string(cpuUsage);
    return usageString;
}

void JasmineGraphInstanceService::collectTrainedModels(instanceservicesessionargs *sessionargs, std::string graphID,
                                                       std::map<std::string, JasmineGraphInstanceService::workerPartitions> graphPartitionedHosts,
                                                       int totalPartitions) {
    int total_threads = totalPartitions;
    std::thread *workerThreads = new std::thread[total_threads];
    int count = 0;
    std::map<std::string, JasmineGraphInstanceService::workerPartitions>::iterator mapIterator;
    for (mapIterator = graphPartitionedHosts.begin(); mapIterator != graphPartitionedHosts.end(); mapIterator++) {
        string hostName = mapIterator->first;
        JasmineGraphInstanceService::workerPartitions workerPartitions = mapIterator->second;
        std::vector<std::string>::iterator it;
        for (it = workerPartitions.partitionID.begin(); it != workerPartitions.partitionID.end(); it++) {
            workerThreads[count] = std::thread(&JasmineGraphInstanceService::collectTrainedModelThreadFunction,
                                               sessionargs, hostName, workerPartitions.port,
                                               workerPartitions.dataPort, graphID, it);
            count++;
        }
    }

    for (int threadCount = 0; threadCount < count; threadCount++) {
        workerThreads[threadCount].join();
        std::cout << "Thread " << threadCount << " joined" << std::endl;
    }
}

int JasmineGraphInstanceService::collectTrainedModelThreadFunction(instanceservicesessionargs *sessionargs,
                                                                   std::string host, int port, int dataPort,
                                                                   std::string graphID, std::string partition) {
    Utils utils;
    bool result = true;
    std::cout << pthread_self() << " host : " << host << " port : " << port << " DPort : " << dataPort << std::endl;
    int sockfd;
    char data[300];
    bool loop = false;
    socklen_t len;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        std::cerr << "Cannot accept connection" << std::endl;
        return 0;
    }

    if (host.find('@') != std::string::npos) {
        host = utils.split(host, '@')[1];
    }

    server = gethostbyname(host.c_str());
    if (server == NULL) {
        std::cerr << "ERROR, no host named " << server << std::endl;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR connecting" << std::endl;
        //TODO::exit
    }
    bzero(data, 301);
    write(sockfd, JasmineGraphInstanceProtocol::HANDSHAKE.c_str(), JasmineGraphInstanceProtocol::HANDSHAKE.size());
    instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::HANDSHAKE, "info");
    bzero(data, 301);
    read(sockfd, data, 300);
    string response = (data);

    response = utils.trim_copy(response, " \f\n\r\t\v");
    if (response.compare(JasmineGraphInstanceProtocol::HANDSHAKE_OK) == 0) {
        instance_logger.log("Received : " + JasmineGraphInstanceProtocol::HANDSHAKE_OK, "info");

        string server_host = sessionargs->hostName;
//        string server_host = utils.getJasmineGraphProperty("org.jasminegraph.server.host");
        write(sockfd, server_host.c_str(), server_host.size());
        instance_logger.log("Sent : " + server_host, "info");

        write(sockfd, JasmineGraphInstanceProtocol::INITIATE_MODEL_COLLECTION.c_str(),
              JasmineGraphInstanceProtocol::INITIATE_MODEL_COLLECTION.size());
        instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::INITIATE_MODEL_COLLECTION, "info");
        bzero(data, 301);
        read(sockfd, data, 300);
        response = (data);
        response = utils.trim_copy(response, " \f\n\r\t\v");

        if (response.compare(JasmineGraphInstanceProtocol::OK) == 0) {
            instance_logger.log("Received : " + JasmineGraphInstanceProtocol::OK, "info");

            string server_host = sessionargs->hostName;
            write(sockfd, server_host.c_str(), server_host.size());
            instance_logger.log("Sent : " + server_host, "info");

            int server_port = sessionargs->port;
            write(sockfd, to_string(server_port).c_str(), to_string(server_port).size());
            instance_logger.log("Sent : " + server_port, "info");

            int server_data_port = sessionargs->dataPort;
            write(sockfd, to_string(server_data_port).c_str(), to_string(server_data_port).size());
            instance_logger.log("Sent : " + server_data_port, "info");

            write(sockfd, graphID.c_str(), (graphID).size());
            instance_logger.log("Sent : Graph ID " + graphID, "info");

            write(sockfd, partition.c_str(), (partition).size());
            instance_logger.log("Sent : Partition ID " + partition, "info");

            write(sockfd, JasmineGraphInstanceProtocol::SEND_FILE_NAME.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_NAME.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_NAME, "info");

            bzero(data, 301);
            read(sockfd, data, 300);
            string fileName = (data);
            instance_logger.log("Received File name: " + fileName, "info");
            write(sockfd, JasmineGraphInstanceProtocol::SEND_FILE_LEN.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_LEN.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_LEN, "info");
            bzero(data, 301);
            read(sockfd, data, 300);
            string size = (data);
            instance_logger.log("Received file size in bytes: " + size, "info");

            write(sockfd, JasmineGraphInstanceProtocol::SEND_FILE_CONT.c_str(),
                  JasmineGraphInstanceProtocol::SEND_FILE_CONT.size());
            instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::SEND_FILE_CONT, "info");
            string fullFilePath =
                    utils.getJasmineGraphProperty("org.jasminegraph.server.instance.datafolder") + "/" + fileName;
            int fileSize = atoi(size.c_str());
            while (utils.fileExists(fullFilePath) && utils.getFileSize(fullFilePath) < fileSize) {
                bzero(data, 301);
                read(sockfd, data, 300);
                response = (data);

                if (response.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                    write(sockfd, JasmineGraphInstanceProtocol::FILE_RECV_WAIT.c_str(),
                          JasmineGraphInstanceProtocol::FILE_RECV_WAIT.size());
                }
            }

            bzero(data, 301);
            read(sockfd, data, 300);
            response = (data);

            if (response.compare(JasmineGraphInstanceProtocol::FILE_RECV_CHK) == 0) {
                instance_logger.log("Received : " + JasmineGraphInstanceProtocol::FILE_RECV_CHK, "info");
                write(sockfd, JasmineGraphInstanceProtocol::FILE_ACK.c_str(),
                      JasmineGraphInstanceProtocol::FILE_ACK.size());
                instance_logger.log("Sent : " + JasmineGraphInstanceProtocol::FILE_ACK, "info");
            }
        }
    } else {
        instance_logger.log("There was an error in the model collection process and the response is :: " + response,
                            "error");
    }

    close(sockfd);
    return 0;
}
