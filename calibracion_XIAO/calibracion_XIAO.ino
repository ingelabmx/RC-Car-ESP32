// ============================================================
//  Calibración — XIAO ESP32-S3 + FS-GR3E + pot de dirección
//  Sube esto PRIMERO, con el motor de dirección DESCONECTADO.
//
//  Arduino IDE: placa "XIAO_ESP32S3", USB CDC On Boot: Enabled
//  Monitor serial a 115200 baudios.
//
//  1) Gira las ruedas a mano de tope a tope -> anota POT min/max
//  2) Mueve el volante del GT3C de tope a tope -> anota PULSO min/max
//  3) Presiona RESET y repite girando DESPACIO para descartar picos.
// ============================================================

const int PIN_CH1 = D1;   // GPIO2  <- GR3E CH1 pin S
const int PIN_POT = D0;   // GPIO1  <- pot VERDE (ADC1)

int potLo = 4095, potHi = 0;
int pulLo = 3000, pulHi = 0;

int leerPot() {
  long suma = 0;
  for (int i = 0; i < 8; i++) suma += analogRead(PIN_POT);
  return suma / 8;
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}   // espera al monitor
  pinMode(PIN_CH1, INPUT);
  analogReadResolution(12);                     // 0-4095
  Serial.println("Calibracion iniciada. Mueve ruedas y volante de tope a tope.");
}

void loop() {
  int pulse = pulseIn(PIN_CH1, HIGH, 30000);
  int pot   = leerPot();

  if (pot < potLo) potLo = pot;
  if (pot > potHi) potHi = pot;
  if (pulse >= 900 && pulse <= 2100) {
    if (pulse < pulLo) pulLo = pulse;
    if (pulse > pulHi) pulHi = pulse;
  }

  Serial.print("pulso=");
  Serial.print(pulse == 0 ? String("SIN SENAL") : String(pulse));
  Serial.print("  pot=");
  Serial.print(pot);
  Serial.print("   | POT min/max: ");
  Serial.print(potLo); Serial.print(" / "); Serial.print(potHi);
  Serial.print("   PULSO min/max: ");
  Serial.print(pulLo); Serial.print(" / "); Serial.println(pulHi);

  delay(100);
}
