#include <painlessMesh.h>

#define MESH_PREFIX   "Redmi 12"
#define MESH_PASSWORD "22345678"
#define MESH_PORT     5555

painlessMesh mesh;
Scheduler userScheduler;

void sendMessage() {
  int adcValue = analogRead(A0);
  String msg = "{\"tipe\":\"cahaya_node\",\"adc\":" + String(adcValue) + "}";
  mesh.sendBroadcast(msg);
  Serial.println("Kirim: " + msg);
}

Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}