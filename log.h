#ifndef LOG_H
#define LOG_H

#include "Arduino.h"
#include "ZoneState.h"

#define LOG_SERIAL_SPEED                115200

#define LOG_DISABLED                    0           // Log desactivado.
#define LOG_MSG                         1           // Logea mensajes
#define LOG_CTRL_JSON                   2           // Log habilitado en formato json.
#define LOG_CTRL_ARDUINO_PLOTTER        3           // Log habilitado en formato arduino serial plotter.


#define PLOTTER_DISABLED         0          // Sin Arduino plotter seleccionado. 
#define PLOTTER_TEMP_HUM         1          // Habilita el trazado de datos de temperatura y humedad al Arduino Plotter.
#define PLOTTER_VPD              2          // Habilita el trazado de datos del VPD al Arduino Plotter.
#define PLOTTER_LIGHT            3          // Habilita el trazado de datos de luz (lux) al Arduino Plotter. 




#define TEMP_STATE_SCALE         2.0                // Valor de escalado del estado cuando se usa con la temperatura.     
#define HUM_STATE_SCALE          3.0                // Valor de escalado del estado cuando se usa con la humedad.
#define VPD_STATE_SCALE          1.0                // Valor de escalado del estado cuando se usa con el VPD.
#define LIGHT_STATE_SCALE       20.0                // Valor de escalado del estado cuando se usa con la luz.  

// Escalas visuales fijas para gráfico de luz (plotter)
#define LIGHT_PLOT_MIN_NIGHT    20.0f    // durante la noche, base de gráfico
#define LIGHT_PLOT_MAX_DAY      80.0f   // durante el día, tope de gráfico

class Clog
{
  public:
    Clog();
    void init( uint8_t );
    void msg( const __FlashStringHelper *fmt, ... );
    void ctrl( uint16_t raw, uint16_t filtered, uint8_t state, uint16_t danger_point );
    void set_level(uint8_t);
    void log_sensor_with_state(const char* name, float value, float min, float max, ZoneState state, float state_scale, const char* state_label);




  private:
    void msg_ctrl( const __FlashStringHelper *fmt, ... );

    uint8_t level;            // Nivel de log (0 = desactivado).
};

#endif // LOG_H
