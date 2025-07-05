// -------------------------
// DEFINICIÓN DE PINES
// -------------------------

// Pines para sensores de línea
#define SENSOR_IZQUIERDA 34   // Sensor infrarrojo izquierdo
#define SENSOR_CENTRO    39   // Sensor infrarrojo central
#define SENSOR_DERECHA   36   // Sensor infrarrojo derecho

// Pines de entradas de control externas
#define PIN_INICIO       35   // Señal de inicio de recorrido
#define QRDETECTADO      21   // Pin que indica si se detectó un QR

// Pines para motores del lado izquierdo (controlados con L298N)
#define IN1_I            26   // Motor izquierdo IN1
#define IN2_I            33   // Motor izquierdo IN2
#define IN3_I            25   // Motor izquierdo IN3
#define IN4_I            32   // Motor izquierdo IN4

// Pines para motores del lado derecho (controlados con L298N)
#define IN1_D            12   // Motor derecho IN1
#define IN2_D            14   // Motor derecho IN2
#define IN3_D            13   // Motor derecho IN3
#define IN4_D            27   // Motor derecho IN4

// Valores de delay para variar la velocidad de los motores
#define DELAY_MIN        15   // Velocidad mínima (delay corto)
#define DELAY_MED        20   // Velocidad media
#define DELAY_MAX        25   // Velocidad máxima (delay largo)
#define DELAY_CENTRO     15   // Delay cuando se sigue línea recta
#define DELAY_PARADO     0    // Delay de parado

// Pines para motores de la torre (eje vertical)
#define IN1_TORRE        19
#define IN2_TORRE        18
#define IN3_TORRE        5
#define IN4_TORRE        17

// Pines para motores de la cremallera (movimiento horizontal)
#define IN1_CREMALLERA   23
#define IN2_CREMALLERA   22
#define IN3_CREMALLERA   1
#define IN4_CREMALLERA   3

// Sensores de posición y detección de caja
#define SWITCH_TORRE         4   // Fin de carrera para torre
#define SWITCH_CREMALLERA   16   // Fin de carrera para cremallera
#define SWITCH_VENTOSA      15   // Detección de caja con sensor en ventosa

// Pines de control de altura y bomba de vacío
#define PIN_ALTURA          0    // Entrada que indica altura alta/baja
#define PIN_BOMBA           2    // Salida para controlar bomba de vacío

// -------------------------
// MATRICES DE PASOS PARA MOTORES PASO A PASO
// -------------------------

// Secuencia de pasos para motores paso a paso (8 pasos - media fase)
const int pasos[8][4] = {
  { 1, 0, 0, 0 },
  { 1, 1, 0, 0 },
  { 0, 1, 0, 0 },
  { 0, 1, 1, 0 },
  { 0, 0, 1, 0 },
  { 0, 0, 1, 1 },
  { 0, 0, 0, 1 },
  { 1, 0, 0, 1 }
};

// Secuencia de pasos para motores paso a paso (4 pasos - onda completa)
const int stepSequence[4][4] = {
  { 1, 0, 1, 0 },  // A+ B+
  { 0, 1, 1, 0 },  // A- B+
  { 0, 1, 0, 1 },  // A- B-
  { 1, 0, 0, 1 }   // A+ B-
};

// -------------------------
// VARIABLES DE CONTROL DE PASOS
// -------------------------

int pasoIzq = 0;           // Paso actual del motor izquierdo
int pasoDer = 0;           // Paso actual del motor derecho

unsigned long tAnteriorIzq = 0; // Tiempo anterior motor izquierdo
unsigned long tAnteriorDer = 0; // Tiempo anterior motor derecho
int delayIzq = DELAY_MIN;       // Delay motor izquierdo
int delayDer = DELAY_MIN;       // Delay motor derecho

const int stepsPerRevolution = 200; // Cantidad de pasos por revolución (NEMA 17)

// -------------------------
// VARIABLES DE ESTADO GENERAL
// -------------------------

bool yaMovio = false;      // Indica si ya realizó un movimiento de altura
float vueltasAltura = 0;   // Vueltas que debe moverse la torre en Z

// -------------------------
// SETUP: CONFIGURACIÓN INICIAL
// -------------------------

void setup() {
  // Configurar sensores de línea como entradas
  pinMode(SENSOR_IZQUIERDA, INPUT);
  pinMode(SENSOR_CENTRO, INPUT);
  pinMode(SENSOR_DERECHA, INPUT);

  // Entradas de control
  pinMode(PIN_INICIO, INPUT_PULLDOWN);
  pinMode(QRdetectado, INPUT);  // Pin QR detectado

  // Configurar motores de línea como salidas
  pinMode(IN1I, OUTPUT);
  pinMode(IN2I, OUTPUT);
  pinMode(IN3I, OUTPUT);
  pinMode(IN4I, OUTPUT);
  pinMode(IN1D, OUTPUT);
  pinMode(IN2D, OUTPUT);
  pinMode(IN3D, OUTPUT);
  pinMode(IN4D, OUTPUT);

  // Motores de torre y cremallera
  pinMode(IN1torre, OUTPUT);
  pinMode(IN2torre, OUTPUT);
  pinMode(IN3torre, OUTPUT);
  pinMode(IN4torre, OUTPUT);

  pinMode(IN1cremallera, OUTPUT);
  pinMode(IN2cremallera, OUTPUT);
  pinMode(IN3cremallera, OUTPUT);
  pinMode(IN4cremallera, OUTPUT);

  // Bomba de vacío
  pinMode(PINbomba, OUTPUT);

  // Switches fin de carrera
  pinMode(SWITCHcremallera, INPUT);
  pinMode(SWITCHtorre, INPUT);
  pinMode(SWITCHventosa, INPUT_PULLDOWN);
  pinMode(PINaltura, INPUT);

  // Hacer homing de cremallera y torre al iniciar
  homingCremallera();
  homingTorre();
}

// -------------------------
// FUNCIONES DE MOVIMIENTO DE MOTORES DE LÍNEA
// -------------------------

void moverMotorIzquierdo() {
  digitalWrite(IN1I, pasos[pasoIzq][0]);
  digitalWrite(IN2I, pasos[pasoIzq][1]);
  digitalWrite(IN3I, pasos[pasoIzq][2]);
  digitalWrite(IN4I, pasos[pasoIzq][3]);
  pasoIzq = (pasoIzq + 1) % 8;  // Avanza al siguiente paso
}

void moverMotorDerecho() {
  digitalWrite(IN1D, pasos[pasoDer][0]);
  digitalWrite(IN2D, pasos[pasoDer][1]);
  digitalWrite(IN3D, pasos[pasoDer][2]);
  digitalWrite(IN4D, pasos[pasoDer][3]);
  pasoDer = (pasoDer + 1) % 8;  // Avanza al siguiente paso
}

void detenerMotores() {
  // Apaga ambos motores
  digitalWrite(IN1I, LOW);
  digitalWrite(IN2I, LOW);
  digitalWrite(IN3I, LOW);
  digitalWrite(IN4I, LOW);
  digitalWrite(IN1D, LOW);
  digitalWrite(IN2D, LOW);
  digitalWrite(IN3D, LOW);
  digitalWrite(IN4D, LOW);
}

// -------------------------
// FUNCIONES PARA MOTORES PASO A PASO TORRE Y CREMALLERA
// -------------------------

void moverPaso(int in1, int in2, int in3, int in4, int paso) {
  // Ejecuta un paso según la secuencia
  digitalWrite(in1, stepSequence[paso][3]);
  digitalWrite(in2, stepSequence[paso][2]);
  digitalWrite(in3, stepSequence[paso][1]);
  digitalWrite(in4, stepSequence[paso][0]);
}

void apagarMotor(int in1, int in2, int in3, int in4) {
  // Apaga los cuatro pines de un motor
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void homingTorre() {
  // Mueve la torre hasta tocar el switch de home
  while (digitalRead(SWITCHtorre) == HIGH) {
    for (int i = 3; i >= 0; i--) {
      moverPaso(IN1torre, IN2torre, IN3torre, IN4torre, i);
      delay(6);
      if (digitalRead(SWITCHtorre) == LOW) break;
    }
  }
  apagarMotor(IN1torre, IN2torre, IN3torre, IN4torre);
}

void homingCremallera() {
  // Mueve la cremallera hasta tocar el switch de home
  while (digitalRead(SWITCHcremallera) == HIGH) {
    for (int i = 0; i < 4; i++) {
      moverPaso(IN1cremallera, IN2cremallera, IN3cremallera, IN4cremallera, i);
      delay(6);
      if (digitalRead(SWITCHcremallera) == LOW) break;
    }
  }
  apagarMotor(IN1cremallera, IN2cremallera, IN3cremallera, IN4cremallera);
}

void moverMotor(int in1, int in2, int in3, int in4, float vueltas) {
  // Mueve el motor indicado una cantidad de vueltas
  int pasos = abs(vueltas * stepsPerRevolution);
  for (int i = 0; i < pasos; i++) {
    int paso = vueltas > 0 ? i % 4 : (3 - (i % 4));
    moverPaso(in1, in2, in3, in4, paso);
    delay(4);
  }
  apagarMotor(in1, in2, in3, in4);
}

void cajaDetectada() {
  // Gira la torre hasta detectar la caja con la ventosa
  while (digitalRead(SWITCHventosa) == LOW) {
    for (int i = 3; i >= 0; i--) {
      moverPaso(IN1torre, IN2torre, IN3torre, IN4torre, i);
      delay(6);
      if (digitalRead(SWITCHventosa) == HIGH) break;
    }
  }
  apagarMotor(IN1torre, IN2torre, IN3torre, IN4torre);
}

// -------------------------
// LOOP PRINCIPAL: LÓGICA DE SEGUIDOR Y RECOLECCIÓN
// -------------------------

void loop() {
  static bool buscandoQR = false;       // Estado: buscando QR
  static unsigned long tiempoPerdidaLinea = 0; // Marca tiempo sin línea
  static bool rutinaCompletada = false; // Estado de recolección

  // Si detecta QR y no completó rutina aún
  if (digitalRead(QRdetectado) == HIGH && !rutinaCompletada) {
    detenerMotores();
    int estadoAltura = digitalRead(PINaltura);
    vueltasAltura = (estadoAltura == LOW) ? 15 : 35;

    moverMotor(IN1torre, IN2torre, IN3torre, IN4torre, vueltasAltura);
    moverMotor(IN1cremallera, IN2cremallera, IN3cremallera, IN4cremallera, -1.5);

    delay(1000);

    cajaDetectada();

    digitalWrite(PINbomba, HIGH); // Prende bomba
    delay(3000);
    moverMotor(IN1torre, IN2torre, IN3torre, IN4torre, 0.5);
    moverMotor(IN1cremallera, IN2cremallera, IN3cremallera, IN4cremallera, 1.5);
    homingTorre();
    digitalWrite(PINbomba, LOW);  // Apaga bomba

    rutinaCompletada = true;
    return;
  }

  // Si deja de detectar QR, resetea rutina
  if (digitalRead(QRdetectado) == LOW) {
    rutinaCompletada = false;
  }

  // Si se activa el pin de inicio
  if (digitalRead(PIN_INICIO) == HIGH) {
    int izq = digitalRead(SENSOR_IZQUIERDA);
    int centro = digitalRead(SENSOR_CENTRO);
    int der = digitalRead(SENSOR_DERECHA);

    if (izq == 0 && centro == 0 && der == 0) {
      // Se salió de la línea, espera QR
      if (!buscandoQR) {
        buscandoQR = true;
        tiempoPerdidaLinea = millis();
        detenerMotores();
        return;
      }

      if (millis() - tiempoPerdidaLinea >= 5000) {
        // Si pasaron 5 segundos, sigue derecho
        delayIzq = DELAY_MIN;
        delayDer = DELAY_MIN;
      } else {
        detenerMotores();
        return;
      }
    } else {
      // Vuelve a detectar línea, ajusta velocidad
      buscandoQR = false;

      if (centro == 1 && izq == 0 && der == 0) {
        delayIzq = delayDer = DELAY_CENTRO;
      } else if (centro == 1 && der == 1 && izq == 0) {
        delayIzq = DELAY_MED;
        delayDer = DELAY_MIN;
      } else if (centro == 1 && izq == 1 && der == 0) {
        delayIzq = DELAY_MIN;
        delayDer = DELAY_MED;
      } else if (izq == 1 && centro == 0 && der == 0) {
        delayIzq = DELAY_MIN;
        delayDer = DELAY_MAX;
      } else if (der == 1 && centro == 0 && izq == 0) {
        delayIzq = DELAY_MAX;
        delayDer = DELAY_MIN;
      } else if (izq == 1 && centro == 1 && der == 1) {
        delayIzq = delayDer = DELAY_MIN;
      }
    }

    unsigned long tAhora = millis();

    if (tAhora - tAnteriorIzq >= delayIzq) {
      moverMotorIzquierdo();
      tAnteriorIzq = tAhora;
    }

    if (tAhora - tAnteriorDer >= delayDer) {
      moverMotorDerecho();
      tAnteriorDer = tAhora;
    }

  } else {
    detenerMotores();
  }
}
