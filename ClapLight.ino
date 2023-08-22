// Connect the LED to port 13 and the audio meters Analog Out to port A0

void setup() {

}

void loop() {
  // Change the 200 as necessary for how loud the clap/snap should be
  if (analogRead(A0) > 200) {
    analogWrite(13, 255);
    delay(10000);
    analogWrite(13, 0);
  }
  else {
    analogWrite(13, 0);
  }
}
