//
// Created by chinthaka on 9/19/18.
//

#ifndef JASMINEGRAPH_JASMINEGRAPHHASHMAPLOCALSTORE_H
#define JASMINEGRAPH_JASMINEGRAPHHASHMAPLOCALSTORE_H

#include "JasmineGraphLocalStore.h"
#include "../util/dbutil/edgestore_generated.h"
#include <flatbuffers/util.h>

using namespace JasmineGraph::Edgestore;


class JasmineGraphHashMapLocalStore:public JasmineGraphLocalStore {
private:
    std::string VERTEX_STORE_NAME = "acacia.nodestore.db";
    std::string EDGE_STORE_NAME = "acacia.edgestore.db";
    std::string ATTRIBUTE_STORE_NAME = "acacia.attributestore.db";

    int graphId;
    int partitionId;
    std::string instanceDataFolderLocation;
    std::map<long,std::unordered_set<long>> localSubGraphMap;

    long vertexCount;
    long edgeCount;
    int *distributionArray;


    std::string getFileSeparator();
    void toLocalSubGraphMap(const EdgeStore *edgeStoreData);
public:
    JasmineGraphHashMapLocalStore(int graphid, int partitionid);
    JasmineGraphHashMapLocalStore(std::string folderLocation);
    inline bool loadGraph() override;
    bool storeGraph() override;
    long getEdgeCount() override;
    long getVertexCount() override;
    void addEdge(long startVid, long endVid) override;
    unordered_set<long> getVertexSet();
    int* getOutDegreeDistribution();
    map<long, long> getOutDegreeDistributionHashMap() override;
    map<long, unordered_set<long>> getUnderlyingHashMap() override;
    void initialize() override;
    void addVertex(string* attributes) override;
};


#endif //JASMINEGRAPH_JASMINEGRAPHHASHMAPLOCALSTORE_H
