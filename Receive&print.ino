#include "painlessMesh.h"
#include <ArduinoJson.h>

#define MESH_PREFIX     "ScaleBot"
#define MESH_PASSWORD   "12345678"
#define MESH_PORT       5555

painlessMesh mesh;

Scheduler userScheduler;

const int MAX_NODES = 4; // Maximum number of nodes you want to track
bool receivedFromNode[MAX_NODES] = {false};

// User stub
void receivedCallback(uint32_t from, String &msg);

void receivedCallback(uint32_t from, String &msg) {
  //Serial.printf("Received from %u: %s\n", from, msg.c_str());

  // Parse the received JSON data
  StaticJsonDocument<256> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, msg);

  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract and print specific values from the JSON data
  String nodenumStr = jsonDocument["nodenum"];
  int nodenum = nodenumStr.charAt(1) - '0'; // Extract the numeric part

  int weight = jsonDocument["weight_" + String(nodenum)];
  int threshold = jsonDocument["thresholdVal_" + String(nodenum)];
  float Underload = jsonDocument["UnderloadVal_" + String(nodenum)];
  float Overload = jsonDocument["OverloadVal_" + String(nodenum)];

  Serial.printf("Node Number : %d\n", nodenum);
  Serial.printf("Weight from %d: %d\n", nodenum, weight);
  Serial.printf("Threshold %d: %d\n", nodenum, threshold);
  Serial.printf("Underload by %d: %.3f\n", nodenum, Underload);
  Serial.printf("Overload by %d: %.3f\n", nodenum, Overload);

  // Mark the node as received
  receivedFromNode[nodenum - 1] = true;

  Serial.print("Data was received from the Following nodes : ");
  for (int i = 0; i < MAX_NODES; i++) {
    Serial.print(receivedFromNode[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.update();
}

void loop() {
  // Run the mesh update
  mesh.update();

  bool allNodesReceived = true;
  for (int i = 0; i < MAX_NODES; i++) {
    if (!receivedFromNode[i]) {
      allNodesReceived = false;
      break;
    }
  }

  if (allNodesReceived) {
    Serial.println("Received data from all nodes.");
  }
}
