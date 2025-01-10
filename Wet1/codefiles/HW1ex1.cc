#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include "hcm.h"
#include "flat.h"
#include <list>
#include <stack>
#include <unordered_set>
using namespace std;

bool verbose = false;
int findDepth(const hcmNode* node);
int countOccurrences(std::stack<hcmCell*>& stack, const std::string& inst_name);
int count(const string& str);
int DFS(hcmCell* cell);
string changeName(string str);


int main(int argc, char **argv) {
  int argIdx = 1;
  int anyErr = 0;
  unsigned int i;
  vector<string> vlgFiles;
  
  if (argc < 3) {
    anyErr++;
  } 

  else {
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
  
  /*direct to file*/
  string fileName = cellName + string(".stat");
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
  
  hcmCell *flatCell = hcmFlatten(cellName + string("_flat"), topCell, globalNodes);
  cout << "-I- Top cell flattened" << endl;
  
  fv << "file name: " << fileName << endl;
  
  
  
  
  
	// section a: find number of instances in top level cell.
  /* assign your answer for section b to topLevelInstanceCounter */
  int topLevelInstanceCounter = 0;
  //---------------------------------------------------------------------------------//
  topLevelInstanceCounter = topCell->getInstances().size();
  //---------------------------------------------------------------------------------//
	fv << "a: " << topLevelInstanceCounter << endl;






	// section b: find all nodes in top module. excluding global nodes (VSS and VDD).
  /* assign your answer for section a to topLevelNodeCounter */
  int topLevelNodeCounter = 0;
  //---------------------------------------------------------------------------------//
  topLevelNodeCounter = topCell->getNodes().size()- globalNodes.size();
  //---------------------------------------------------------------------------------//
	fv << "b: " << topLevelNodeCounter << endl;









	//section c: find cellName instances in folded model.
  /* assign your answer for section c to cellNameFoldedCounter */
  int cellNameFoldedCounter = 0;
  //---------------------------------------------------------------------------------//
  stack<hcmCell*> folded; //stack to iterate all instances
  folded.push(topCell);
  cellNameFoldedCounter = countOccurrences(folded, "or3");
  //---------------------------------------------------------------------------------//
	fv << "c: " << cellNameFoldedCounter << endl;
	
	
	
	
	
	
	
	

	//section d: find cellName instances in entire hierarchy, using flat cell.
  /* assign your answer for section d to cellNameFlatendCounter */
  int cellNameFlatendCounter = 0;
  //---------------------------------------------------------------------------------//
 const auto& instances = flatCell->getInstances();  // Get all instances in the flattened cell
for (const auto& iI : instances) {
    hcmInstance* instance = iI.second;  // Get the instance
    if (instance->masterCell()->getName() == "or3") {
        cellNameFlatendCounter++;  // Increment counter if the instance is "or3"
    }
}
  //---------------------------------------------------------------------------------//
	fv << "d: " << cellNameFlatendCounter << endl;
	
	
	
	
	
	
	
	
	

	//section e: find the deepest reach of a top level node.
  /* assign your answer for section e to deepestReach */
  int deepestReach = 1;
  //---------------------------------------------------------------------------------//
const map<string, hcmNode*> nodes = topCell->getNodes(); // deepest reach of node that was sent to the recursive function
// Go over the TopCell nodes map using iterators
for (map<string, hcmNode*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
    hcmNode* node = it->second;
    if (!(node->getName() == "VSS" || node->getName() == "VDD")){
		int nodedepth = findDepth(node);
		if (nodedepth > deepestReach)
			deepestReach = nodedepth;
    }
}

  //---------------------------------------------------------------------------------//
	fv << "e: " << deepestReach << endl;
	
	
	
	
	
	
	
	
	

	//section f: find hierarchical names of deepest reaching nodes.
  /* assign your answer for section f to listOfHierarchicalNameOfDeepestReachingNodes */
  list<string> listOfHierarchicalNameOfDeepestReachingNodes;
  //---------------------------------------------------------------------------------//
map<string, hcmInstance*> flatInstances = flatCell->getInstances();
int maxReach = DFS(topCell);
for (const auto& instancePair : flatInstances) {
    hcmInstance* instance = instancePair.second;
    map<string, hcmInstPort*> instancePorts = instance->getInstPorts();
    for (const auto& portPair : instancePorts) {
        string instanceName = portPair.second->getName();
        int currentDepth = count(instanceName);
        if (currentDepth == maxReach) {
            string formattedName = changeName(instanceName);
            listOfHierarchicalNameOfDeepestReachingNodes.push_back(formattedName);
        }
    }
}

listOfHierarchicalNameOfDeepestReachingNodes.sort();

  //---------------------------------------------------------------------------------//
  
  
  
  
  
  
  
  for (auto it : listOfHierarchicalNameOfDeepestReachingNodes) {
    // erase the '/' in the beginning of the hierarchical name. 
    // i.e. the name in listOfHierarchicalNameOfDeepestReachingNodes should be "/i1/i2/i3/i5/N1", 
    // and the printed name should be "i1/i2/i3/i5/N1".
    it.erase(0,1); 
    fv << it << endl;
  }

  return(0);
}









int count(const string& str) {
    int count = 0;
    for (char ch : str)
        if (ch == '/' || ch == '%')
            count++;
    return count+1;
}

string changeName(string str) {
    for (char& ch : str) {
        if (ch == '%') {
            ch = '/';
        }
    }
    return ('/' + str);
}




int DFS(hcmCell* cell) {
    if (!cell) return 0;  // Handle null pointer safely

    auto instMap = cell->getInstances();  // Use auto for type inference
    if (instMap.empty()) {
        return 1;
    }

    int maxDepth = 0;
    for (std::map<std::string, hcmInstance*>::const_iterator it = instMap.begin(); it != instMap.end(); ++it) {
        int currDepth = DFS(it->second->masterCell());  // Access via iterator
        maxDepth = std::max(maxDepth, currDepth);  // Track the maximum depth
    }

    return 1 + maxDepth;  // Add 1 to the maximum depth to account for the current cell
}






int findDepth(const hcmNode* node) {
    if (node->getInstPorts().empty()) {
        return 1;
    }
    int maxReach = 0;
    for (const auto& instPortPair : node->getInstPorts()) {
        hcmInstPort* instPort = instPortPair.second;
        hcmNode* currNode = instPort->getInst()->masterCell()->getNode(instPort->getPort()->getName());

        if (currNode != nullptr) {
            if (!(currNode->getName() == "VSS" || currNode->getName() == "VDD")) {
            int currReach = findDepth(currNode);
            if (currReach > maxReach)
                maxReach = currReach;
			}
        }
    }
    return maxReach + 1;
}






int countOccurrences(std::stack<hcmCell*>& stack, const std::string& inst_name) {
    int counter = 0;
    std::unordered_set<hcmCell*> visited_cells;
    while (!stack.empty()) {
        hcmCell* curr_cell = stack.top();
        stack.pop();
        
        if (visited_cells.find(curr_cell) != visited_cells.end()) {
            continue;
        }
    
        visited_cells.insert(curr_cell);
        const auto& instances = curr_cell->getInstances();
        for (const auto& iI : instances) {
            hcmInstance* instance = iI.second;
            if (instance->masterCell()->getName() == inst_name) {
                counter++;  // Found a matching instance
            } else {
                hcmCell* master_cell = instance->masterCell();
                if (visited_cells.find(master_cell) == visited_cells.end()) {
                    stack.push(master_cell);
                }
            }
        }
    } 
    return counter;
}
	
