#pragma once
#include <stdio.h>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

struct XMLNode {
	string tag;
	map<string, string> attributes;
	string data;
	vector<XMLNode*> childNodes;
};

XMLNode* findNode(XMLNode* root, string tag, string attribute = "", string value = "") {
    if(root == NULL) return NULL;
    vector<XMLNode*> queue;
    queue.push_back(root);
    while(!queue.empty()) {
        XMLNode* curNode = queue.front();
        queue.erase(queue.begin());
        for(int i = 0; i < curNode->childNodes.size(); i++) {
            bool matchAttribute = attribute == "" || (curNode->childNodes[i]->attributes.find(attribute) != curNode->childNodes[i]->attributes.end() && curNode->childNodes[i]->attributes[attribute] == value);
            if(curNode->childNodes[i]->tag == tag && matchAttribute) {
                return curNode->childNodes[i];
            }
            queue.push_back(curNode->childNodes[i]);
        }
    }
    return NULL;
}

XMLNode* loadNode(string& contents, int& i, XMLNode* parent) {
    while(contents[i] != '\0') {
        // skip comments
        if(contents[i] == '<' && contents[i + 1] == '!') {
            while(contents[i] != '\0' && contents[i] != '>' && contents[i - 1] != '-' && contents[i - 2] != '-') i++; // go to end of comment
            continue;
        }
        int startIndex = i;
        if(contents[i] == '<') {
            // this is for sure the start of a new node
            XMLNode* node = new XMLNode();
            if(parent != NULL)
                parent->childNodes.push_back(node);
            // get the tag
            startIndex = i + 1;
            while(contents[i] != '>' && contents[i] != ' ' && contents[i] != '\t' && contents[i] != '\0') {
                i++;
            }
            node->tag = contents.substr(startIndex, i - startIndex);
            // get attributes
            while(contents[i] == ' ' || contents[i] == '\t') i++; // get to next non whitespace
            startIndex = i;
            int prevEqualsIndex = startIndex;
            while(contents[i] != '>' && contents[i] != '\0') {
                if(contents[i] == '=') {
                    prevEqualsIndex = i;
                }
                else if(contents[i] == ' ' || contents[i] == '\t') {
                    // parse attribute
                    string attribute = contents.substr(startIndex, prevEqualsIndex - startIndex);
                    string value = contents.substr(prevEqualsIndex + 2, i - prevEqualsIndex - 3);
                    node->attributes[attribute] = value;
                    while(contents[i] == ' ' || contents[i] == '\t') i++; // get to next non whitespace
                    startIndex = i;
                    i--;
                }
                i++;
            }
            // parse last attribute of this node
            if(i != startIndex) {
                string attribute = contents.substr(startIndex, prevEqualsIndex - startIndex);
                int offset = 0;
                if(contents[i - 1] == '/') offset = 1;
                string value = contents.substr(prevEqualsIndex + 2, i - prevEqualsIndex - 3 - offset);
                node->attributes[attribute] = value;
            }
            // handle tag and attribute only nodes
            if(contents[i] == '>' && contents[i - 1] == '/') {
                i++;
                return node;
            }
            // get data / childNodes
            if(contents[i] != '\0') {
                i++; // go 1 past the >
                startIndex = i;
                while(contents[i] != '<' && contents[i] != '\0') i++; // get to next start or end tag
                // handle end tag for this node
                if(contents[i] == '<' && contents[i + 1] == '/') {
                    // remove leading or trailing space
                    int soff = startIndex, eoff = i - startIndex;
                    if(contents[soff] == ' ') {
                        soff++;
                        eoff--;
                    }
                    if(contents[soff + eoff - 1] == ' ') eoff--;
                    // get data
                    node->data = contents.substr(soff, eoff);
                    while(contents[i] != '>' && contents[i] != '\0') i++; // go to end of the end tag
                    i++;
                    return node;
                }
                // get childNodes
                while(contents[i] != '\0') {
                    loadNode(contents, i, node);
                    while(contents[i] == ' ' || contents[i] == '\t') i++; // get to next non whitespace
                    // handle end tag for this node
                    if(contents[i] != '\0' && contents[i] == '<' && contents[i + 1] == '/') {
                        while(contents[i] != '>' && contents[i] != '\0') i++; // go to end of the end tag
                        if(contents[i] != '\0')
                            i++;
                        return node;
                    }
                }
            }
            if(contents[i] == '\0') {
                return node;
            }
        }
        i++;
    }
    return NULL;
}

XMLNode* loadXmlFile(const char* path) {
	std::ifstream fileStream(path, std::ios::in);
	if(!fileStream.is_open()) {
		printf("Failed to open %s.\n", path);
		return NULL;
	}
    string contents = "";
	std::string line = "";
    getline(fileStream, line);
    if(line.size() > 1 && line[1] != '?') {
        contents = line;
    }
	while(getline(fileStream, line))
		contents += line + " ";
    // replace tabs and multi-spaces with just 1 space
    int firstSpaceInBlock = 0;
    bool prevCharWasSpace = false;
    for(int i = contents.length() - 1; i >= 0; i--) {
        if(contents[i] == '\t') {
            contents[i] = ' ';
        }
        if(contents[i] == ' ' && !prevCharWasSpace) {
            firstSpaceInBlock = i;
            prevCharWasSpace = true;
        }
        else if(contents[i] != ' ' && prevCharWasSpace && firstSpaceInBlock != i + 1) {
            // close multi-space block
            contents.erase(contents.begin() + i + 2, contents.begin() + firstSpaceInBlock + 1);
        }
        if(contents[i] != ' ') {
            prevCharWasSpace = false;
        }
    }
    int i = 0;
    XMLNode* node = loadNode(contents, i, NULL);
	fileStream.close();
	return node;
}

void freeNode(XMLNode* node) {
    if(node == NULL) return;
    // for(int i = 0; i < level; i++) {
    //     printf(" ");
    // }
    // printf("%s, ", node->tag.c_str());
    // for(auto i = node->attributes.begin(); i != node->attributes.end(); i++) {
    //     printf("%s=%s, ", i->first.c_str(), i->second.c_str());
    // }
    // printf("%s\n", node->data.c_str());
    for(int i = 0; i < node->childNodes.size(); i++) {
        freeNode(node->childNodes[i]);
    }
    delete node;
}

// int main() {
//     XMLNode* node = loadXmlFile("C:\\Users\\Noxide\\Downloads\\game_assets\\Monk.dae");
//     printf("yo\n");
//     freeNode(node, 0);
//     // printf("wut\n");

//     return 0;
// }
