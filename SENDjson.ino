#include "painlessMesh.h"
#include <ArduinoJson.h>

#define MESH_PREFIX     "ScaleBot"
#define MESH_PASSWORD   "12345678"
#define MESH_PORT       5555

painlessMesh mesh;

Scheduler userScheduler; // To control your personal task

// User stub
void receivedCallback(uint32_t from, String &msg);

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());

  // Parse the received JSON data
  StaticJsonDocument<256> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, msg);

  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract and print specific values from the JSON data
  int nodenum = jsonDocument["nodenum"];
  int weight = jsonDocument["weight_" + String(nodenum)];
  int threshold = jsonDocument["threshold_" + String(nodenum)];
  float Underload = jsonDocument["Underloadval_" + String(nodenum)];
  float Overload = jsonDocument["Overloadval_" + String(nodenum)];

  Serial.printf("Mesh ID: %d\n", nodenum);
  Serial.printf("Weight: %d\n", weight);
  Serial.printf("Threshold: %d\n", threshold);
  Serial.printf("Underload by: %f\n", Underload);
  Serial.printf("Overload by: %f\n", Overload);
}

void setup() {
  Serial.begin(115200);

  // Initialize PainlessMesh and callbacks
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  // Run the mesh network
  mesh.update();
}

void loop() {
  // Run the mesh update
  mesh.update();

  // You can call a function here to retrieve data from a specific node
  int nodeIndexToRetrieve = 1; // Change this index as needed
  retrieveDataFromNode(nodeIndexToRetrieve);
}

void retrieveDataFromNode(int nodeIndex) {
  // Create a JSON request to get data from a specific node
  StaticJsonDocument<64> requestJson;
  requestJson["request"] = "getData";
  requestJson["nodeIndex"] = nodeIndex;

  // Serialize the JSON request to a string
  String jsonString;
  serializeJson(requestJson, jsonString);

  // Send the JSON request to the node
  mesh.sendBroadcast(jsonString);

  Serial.printf("Sent data retrieval request to node %d\n", nodeIndex);
}
