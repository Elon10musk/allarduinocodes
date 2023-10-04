#include "painlessMesh.h"
#include <ArduinoJson.h>

#define MESH_PREFIX     "ScaleBot"
#define MESH_PASSWORD   "12345678"
#define MESH_PORT       5555

painlessMesh mesh;

String nodenum = "_1"; 

int meshid = mesh.getNodeId();
#define meshId meshid   // Store the mesh ID of this node
Scheduler userScheduler; // to control your personal task

// User stubs
void sendJsonData();

Task taskSendJsonData(1 * TASK_SECOND, TASK_FOREVER, &sendJsonData);

void sendJsonData() {
  // Create a JSON document
  StaticJsonDocument<256> jsonDocument;
  
  // Add sensor data to the JSON document
  jsonDocument["nodenum"] = nodenum;
  jsonDocument["weight"+nodenum] = 888;
  jsonDocument["thresholdVal"+nodenum] = 90;
  jsonDocument["UnderloadVal"+nodenum] = 0.444;
  jsonDocument["OverloadVal"+nodenum] = 1.077;
  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  // Send JSON data over the mesh
  mesh.sendBroadcast(jsonString);

  taskSendJsonData.setInterval(1 * TASK_SECOND);  // Send data every 10 minutes
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}
void setup() {
  Serial.begin(115200);

  // Initialize PainlessMesh and callbacks
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Add tasks to the scheduler
  userScheduler.addTask(taskSendJsonData);

  // Enable the tasks
  taskSendJsonData.enable();
}

void loop() {
  // Run the mesh update and user scheduler
  mesh.update();
  userScheduler.execute();
}


//node 2 receiving :-
//.................................................................................................................................................................................................
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
  int weight = jsonDocument["weight_1"];
  int threshold = jsonDocument["threshold_1"];
  float Underload = jsonDocument["Underloadval_1"];
  float Overload = jsonDocument["Overloadval_1"];

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
}
