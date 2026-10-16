// ============================================================
//  Dirección de lazo cerrado — Seeed XIAO ESP32-S3
//  FS-GT3C -> FS-GR3E (CH1) -> XIAO -> TB6612FNG -> servo de 5 hilos
//
//  La tracción NO pasa por aquí: el ESC va directo al CH2 del GR3E.
//
//  Arduino IDE: placa "XIAO_ESP32S3" (paquete esp32 de Espressif 3.x)
//  USB CDC On Boot: Enabled (para ver el Serial)
//
//  Conexiones:
//    D1 (GPIO2) <- GR3E CH1 pin S     (directo, sin transistor)
//    D0 (GPIO1) <- pot VERDE          (ROJO -> 3V3, BLANCO -> GND)
//    D3 (GPIO4) -> TB6612 PWMA        (PWM 20 kHz)
//    D4 (GPIO5) -> TB6612 AIN1
//    D5 (GPIO6) -> TB6612 AIN2
//    3V3        -> TB6612 VCC + STBY, pot ROJO
//    5V         <- BEC del ESC (fila VCC del GR3E)
//    AO1 -> motor AMARILLO, AO2 -> motor NEGRO
// ============================================================

// ---------------- Pines ----------------
const int PIN_CH1 = D1;
const int PIN_POT = D0;
const int PWMA    = D3;
const int AIN1    = D4;
const int AIN2    = D5;

const int PWM_FREQ = 20000;   // 20 kHz, fuera del rango audible
const int PWM_RES  = 8;       // 0-255

// ---------------- Calibración ----------------
// Valores ESTIMADOS a partir de la calibración del R4 (pot a 3.3V,
// ADC de 12 bits). Reemplázalos con los de calibracion_XIAO.ino.
// Se mantiene el mismo orden que en el R4 (potMin > potMax).
int potMin   = 3156;
int potMax   = 586;
int pulseMin = 1003;
int pulseMax = 2016;

// Margen para no empujar contra los topes mecánicos (cuentas ADC)
const int MARGEN_TOPE = 120;

// Si la dirección se va al tope y se queda empujando, cambia a true
const bool INVERTIR_MOTOR = false;

// ---------------- Control (escalado a ADC de 12 bits) ----------------
const int   DEADBAND = 120;   // zona muerta. Sube si tiembla en el centro
const float KP       = 0.45;  // ganancia. Sube si responde lento, baja si oscila
const int   PWM_MIN  = 90;    // mínimo para vencer la fricción (0-255)
const int   PWM_MAX  = 255;   // máximo

// Failsafe: si no hay pulso válido en este tiempo, se suelta el motor
const unsigned long TIMEOUT_SENAL_MS = 100;

// Depuración por serial (ponlo en false cuando ya funcione)
const bool DEBUG = true;

// LED de usuario del XIAO (GPIO21, activo en bajo): encendido = señal OK
const int LED_USER = LED_BUILTIN;

// ---------------- Estado ----------------
unsigned long ultimoPulsoOk = 0;

// ------------------------------------------------------------
void motorSoltar() {
  ledcWrite(PWMA, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
}

void motorFrenar() {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
  ledcWrite(PWMA, 255);
}

void motorMover(int error) {
  bool haciaArriba = (error > 0);
  if (INVERTIR_MOTOR) haciaArriba = !haciaArriba;

  int pwm = constrain((int)(abs(error) * KP), PWM_MIN, PWM_MAX);

  digitalWrite(AIN1, haciaArriba ? HIGH : LOW);
  digitalWrite(AIN2, haciaArriba ? LOW  : HIGH);
  ledcWrite(PWMA, pwm);
}

int leerPot() {
  // Promedio de 8 lecturas: el ADC del ESP32 es más ruidoso
  long suma = 0;
  for (int i = 0; i < 8; i++) suma += analogRead(PIN_POT);
  return suma / 8;
}

// ------------------------------------------------------------
void setup() {
  pinMode(PIN_CH1, INPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(LED_USER, OUTPUT);
  analogReadResolution(12);

  ledcAttach(PWMA, PWM_FREQ, PWM_RES);   // API del core esp32 3.x
  motorSoltar();

  if (DEBUG) Serial.begin(115200);
}

void loop() {
  int pulse = pulseIn(PIN_CH1, HIGH, 30000);
  bool senalOk = (pulse >= 900 && pulse <= 2100);
  digitalWrite(LED_USER, senalOk ? LOW : HIGH);   // activo en bajo

  // ---- Failsafe por software ----
  if (senalOk) {
    ultimoPulsoOk = millis();
  } else {
    if (millis() - ultimoPulsoOk > TIMEOUT_SENAL_MS) {
      motorSoltar();
      if (DEBUG) Serial.println("SIN SENAL -> motor suelto");
    }
    return;
  }

  // ---- Objetivo: pulso del volante -> posición del pot ----
  pulse = constrain(pulse, min(pulseMin, pulseMax), max(pulseMin, pulseMax));
  int target = map(pulse, pulseMin, pulseMax, potMin, potMax);
  target = constrain(target,
                     min(potMin, potMax) + MARGEN_TOPE,
                     max(potMin, potMax) - MARGEN_TOPE);

  // ---- Lazo P con freno activo ----
  int pos   = leerPot();
  int error = target - pos;

  if (abs(error) < DEADBAND) {
    motorFrenar();
  } else {
    motorMover(error);
  }

  if (DEBUG) {
    static unsigned long ultimoPrint = 0;
    if (millis() - ultimoPrint > 150) {
      ultimoPrint = millis();
      Serial.print("pulso="); Serial.print(pulse);
      Serial.print("  target="); Serial.print(target);
      Serial.print("  pos="); Serial.print(pos);
      Serial.print("  error="); Serial.println(error);
    }
  }
}
