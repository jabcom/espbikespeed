#define UPDATE_FREQUENCY 1;
#define GAME_IP "###";
#define DATA_AVERAGE_COUNT 3;

float speedDataArr[DATA_AVERAGE_COUNT];
short speedDataIndex = 0;
float calculatedSpeed;

void setup() {
  //Join Network
  //Setup interupt
}

void loop() {
  float runningTotal = 0;
  for (short i = 0; i < DATA_AVERAGE_COUNT; i++) {
    runningTotal += speedDataArr[i];
  }
  calculatedSpeed = runningTotal / DATA_AVERAGE_COUNT;
  print String(calculatedSpeed)
  delay(int((1/120) * 1000);
}
