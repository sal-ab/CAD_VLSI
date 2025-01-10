#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "hcm.h"
#include "flat.h"

using namespace std;

bool verbose = false;

class InstRankCalculator {
private:
    struct InstanceData {
        hcmInstance* instance;
        int rank = -1;
        bool isRankComputed = false; 
    };

    vector<InstanceData> instanceDataList;  

int computeMaxRank(int index) {
    int maxRank = -1;
    auto& currentInstanceData = instanceDataList[index];
    
    for (const auto& portPair : currentInstanceData.instance->getInstPorts()) {
        hcmInstPort* instancePort = portPair.second;
		
        if (instancePort->getPort()->getDirection() != IN) continue;
		
        hcmInstance* previousInstance = getPreviousInstance(instancePort);
        if (!previousInstance) {
            const string& nodeName = instancePort->getNode()->getName();
            if (nodeName != "VDD" && nodeName != "VSS") {
                currentInstanceData.rank = max(currentInstanceData.rank, 0);
            }
            continue;
        }

        int prevIndex = find_if(instanceDataList.begin(), instanceDataList.end(),
                                 [previousInstance](const InstanceData& data) {
                                     return data.instance == previousInstance;
                                 }) != instanceDataList.end()
                                ? static_cast<int>(distance(instanceDataList.begin(), find_if(instanceDataList.begin(), instanceDataList.end(),
                                                                                             [previousInstance](const InstanceData& data) {
                                                                                                 return data.instance == previousInstance;
                                                                                             })))
                                : -1;

        if (prevIndex == -1) continue;

        if (!instanceDataList[prevIndex].isRankComputed) {
            maxRank = computeMaxRank(prevIndex);
        } else {
            maxRank = instanceDataList[prevIndex].rank;
        }

        if (maxRank != -1) {
            currentInstanceData.rank = max(maxRank + 1, currentInstanceData.rank);
        }
    }

    currentInstanceData.isRankComputed = true;
    return currentInstanceData.rank;
}




    hcmInstance* getPreviousInstance(hcmInstPort* instancePort) const {
        for (const auto& portPair : instancePort->getNode()->getInstPorts()) {
            hcmInstPort* port = portPair.second;
            if (port != instancePort && port->getPort()->getDirection() == OUT) {
                return port->getInst();
            }
        }
        return nullptr;
    }


public:
    void initializeInstanceData(hcmCell* cell) {
        instanceDataList.clear();  
        for (const auto& instancePair : cell->getInstances()) {
            hcmInstance* instance = instancePair.second;  
            instanceDataList.push_back({instance, -1, false});
        }
    }


    void computeAndSortRanks(vector<pair<int, string>>& sortedInstanceRanks) {
        for (size_t i = 0; i < instanceDataList.size(); ++i) {
            if (!instanceDataList[i].isRankComputed) {
                instanceDataList[i].rank = computeMaxRank(static_cast<int>(i));
            }
        }

        for (const auto& instanceData : instanceDataList) {
            if (instanceData.rank != -1) {
                sortedInstanceRanks.emplace_back(instanceData.rank, instanceData.instance->getName());
            }
        }

		sort(sortedInstanceRanks.begin(), sortedInstanceRanks.end(), 
			[](const pair<int, string>& a, const pair<int, string>& b) {
				return (a.first < b.first) || (a.first == b.first && a.second < b.second);
			});

    }
};




int main(int argc, char **argv) {
  int argIdx = 1;
  int anyErr = 0;
  unsigned int i;
  vector<string> vlgFiles;
  
  if (argc < 3) {
    anyErr++;
  } else {
    if (!strcmp(argv[argIdx], "-v")) {
      argIdx++;
      verbose = true;
    }
    for (;argIdx < argc; argIdx++) {
      vlgFiles.push_back(argv[argIdx]);
    }
    
    if (vlgFiles.size() < 2) {
      cerr << "-E- At least top-level and single verilog file required for spec model" << endl;
      anyErr++;
    }
  }

  if (anyErr) {
    cerr << "Usage: " << argv[0] << "  [-v] top-cell file1.v [file2.v] ... \n";
    exit(1);
  }
 
  set< string> globalNodes;
  globalNodes.insert("VDD");
  globalNodes.insert("VSS");
  
  hcmDesign* design = new hcmDesign("design");
  string cellName = vlgFiles[0];
  for (i = 1; i < vlgFiles.size(); i++) {
    printf("-I- Parsing verilog %s ...\n", vlgFiles[i].c_str());
    if (!design->parseStructuralVerilog(vlgFiles[i].c_str())) {
      cerr << "-E- Could not parse: " << vlgFiles[i] << " aborting." << endl;
      exit(1);
    }
  }
  

  /*direct output to file*/
  string fileName = cellName + string(".rank");
  ofstream fv(fileName.c_str());
  if (!fv.good()) {
    cerr << "-E- Could not open file:" << fileName << endl;
    exit(1);
  }

  hcmCell *topCell = design->getCell(cellName);
  if (!topCell) {
    printf("-E- could not find cell %s\n", cellName.c_str());
    exit(1);
  }
  
  fv << "file name: " << fileName << endl;
  
  hcmCell *flatCell = hcmFlatten(cellName + string("_flat"), topCell, globalNodes);

  /* assign your answer for HW1ex2 to maxRankVector 
   * maxRankVector is a vector of pairs of type <int, string>,
   * int is the rank of the instance,
   * string is the name of the instance
   * Note - maxRankVector should be sorted.
  */
  vector<pair<int, string>> maxRankVector;
  //---------------------------------------------------------------------------------//
	InstRankCalculator calculator;
	calculator.initializeInstanceData(flatCell);
    calculator.computeAndSortRanks(maxRankVector);
  //---------------------------------------------------------------------------------//
  for(auto itr = maxRankVector.begin(); itr != maxRankVector.end(); itr++){
		fv << itr->first << " " << itr->second << endl;
	}

  return(0);
}
