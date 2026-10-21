/*
  Practica 4.1.1-A  Driver de software para un relevador
  Placa: Arduino UNO R4 WiFi

  Una sola clase controla dos actuadores con logica opuesta:
    Motor por L298N (D7) -> activo en ALTO
    LED al reves  (D6)   -> activo en BAJO

  Comandos del Monitor Serie (9600, con "Nueva linea"):
    M1 / M0  encender o apagar el motor
    L1 / L0  encender o apagar el LED
    E        mostrar el estado de ambos

  Sin delay(). El programa principal nunca llama digitalWrite().
*/

class Relevador {
  public:
    Relevador(uint8_t pin, bool activoEnBajo, uint32_t tMinimo = 1000)
      : _pin(pin),
        _activoEnBajo(activoEnBajo),
        _tMinimo(tMinimo),
        _encendido(false),
        _ultimoCambio(0) {}

    void iniciar() {
      // Se escribe el nivel de APAGADO antes de volver el pin salida.
      // Si no, el pin arranca en LOW y un actuador activo en bajo
      // daria un pulso de encendido durante el arranque.
      digitalWrite(_pin, _nivelPara(false));
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, _nivelPara(false));

      _encendido    = false;
      _ultimoCambio = millis();
    }

    bool encender() { return _cambiarA(true);  }
    bool apagar()   { return _cambiarA(false); }

    bool estaEncendido() const { return _encendido; }

    // Cuanto falta para poder conmutar otra vez
    uint32_t faltaMs() const {
      uint32_t t = millis() - _ultimoCambio;
      return (t >= _tMinimo) ? 0 : (_tMinimo - t);
    }

  private:
    const uint8_t  _pin;
    const bool     _activoEnBajo;
    const uint32_t _tMinimo;

    bool     _encendido;
    uint32_t _ultimoCambio;

    // Aqui vive toda la traduccion activo-alto / activo-bajo
    uint8_t _nivelPara(bool encendido) const {
      return (encendido != _activoEnBajo) ? HIGH : LOW;
    }

    bool _cambiarA(bool destino) {
      if (_encendido == destino) return true;   // ya esta ahi, no conmuta

      if (millis() - _ultimoCambio < _tMinimo) return false;  // rechazada

      digitalWrite(_pin, _nivelPara(destino));
      _encendido    = destino;
      _ultimoCambio = millis();
      return true;
    }
};

// ---------------------------------------------------

Relevador motor(7, false);   // L298N IN1 - activo en ALTO
Relevador led  (6, true);    // LED al reves - activo en BAJO

char    linea[8];
uint8_t n = 0;

// ---------------------------------------------------

void intentar(Relevador &r, bool encender, const char *nombre) {
  bool ok = encender ? r.encender() : r.apagar();

  if (ok) {
    Serial.print(nombre);
    Serial.println(encender ? " ENCENDIDO" : " APAGADO");
  } else {
    Serial.print("Orden rechazada: espere ");
    Serial.print(r.faltaMs());
    Serial.println(" ms entre cambios");
  }
}

void mostrarEstado() {
  Serial.println(F("--- ESTADO ---"));
  Serial.print(F("Motor: "));
  Serial.println(motor.estaEncendido() ? "ENCENDIDO" : "APAGADO");
  Serial.print(F("LED:   "));
  Serial.println(led.estaEncendido() ? "ENCENDIDO" : "APAGADO");
  Serial.println();
}

void procesar(const char *cmd) {
  if      (!strcmp(cmd, "M1")) intentar(motor, true,  "Motor");
  else if (!strcmp(cmd, "M0")) intentar(motor, false, "Motor");
  else if (!strcmp(cmd, "L1")) intentar(led,   true,  "LED");
  else if (!strcmp(cmd, "L0")) intentar(led,   false, "LED");
  else if (!strcmp(cmd, "E" )) mostrarEstado();
  else {
    Serial.print(F("Comando desconocido: "));
    Serial.println(cmd);
  }
}

void leerSerie() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (n > 0) {
        linea[n] = '\0';
        procesar(linea);
        n = 0;
      }
    }
    else if (n < sizeof(linea) - 1) {
      linea[n++] = toupper(c);
    }
  }
}

// ---------------------------------------------------

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 3000);

  motor.iniciar();      // estado seguro: ambos apagados
  led.iniciar();

  Serial.println(F("Driver de relevador listo"));
  Serial.println(F("Comandos: M1 M0 L1 L0 E"));
  Serial.println();
}

void loop() {
  leerSerie();          // unica tarea, nunca bloquea
}