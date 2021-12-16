#define LDR     A0 
void setup() {
Serial.begin(115200);  
  pinMode(LDR, INPUT); 
}

void loop() {
  Serial.println(analogRead(LDR)); 
} 
