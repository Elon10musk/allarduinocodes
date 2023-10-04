//node 1 code sending :-
#include "painlessMesh.h"
#include <ArduinoJson.h>

#define MESH_PREFIX     "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

painlessMesh mesh;

int meshid = mesh.getNodeId();
#define meshId meshid   // Store the mesh ID of this node
Scheduler userScheduler; // to control your personal task

// User stubs
void sendJsonData();

Task taskSendJsonData(TASK_SECOND * 1 , TASK_FOREVER, &sendJsonData);
//Task taskSendJsonData(10 * TASK_MINUTE, TASK_FOREVER, &sendJsonData);

void sendJsonData() {
  // Create a JSON document
  StaticJsonDocument<256> jsonDocument;
  
  // Add sensor data to the JSON document
  jsonDocument["meshID"] = mesh.getNodeId();
  jsonDocument["weight+meshId"] = 888;
  jsonDocument["threshold+meshId"] = 90;

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  // Send JSON data over the mesh
  mesh.sendBroadcast(jsonString);

  taskSendJsonData.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));  // Send data every 10 (minutes 10 * TASK_MINUTE)
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

#define MESH_PREFIX     "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
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
  int meshID = jsonDocument["meshID"];
  int weight = jsonDocument["weight+meshId"];
  int threshold = jsonDocument["threshold+meshId"];

  Serial.printf("Mesh ID: %d\n", meshID);
  Serial.printf("Weight: %d\n", weight);
  Serial.printf("Threshold: %d\n", threshold);
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
