# ESP32 LittleFS Logger — fs-logger-demo

(Contenido previo del README...)


## 📹 Videos de prueba

En la carpeta [`videos/`](./videos) se incluyen dos capturas de pruebas en vivo:

<h3>Simulación de rotación diaria</h3>
<video controls width="640">
  <source src="https://github.com/theinsideshine/fs_logger_demo/raw/main/videos/simulate-rotate-log.mp4" type="video/mp4">
  Tu navegador no soporta video HTML5. <a href="https://github.com/theinsideshine/fs_logger_demo/raw/main/videos/simulate-rotate-log.mp4">Descargar MP4</a>
</video>

<h3>Simulación de low-water</h3>
<video controls width="640">
  <source src="https://github.com/theinsideshine/fs_logger_demo/raw/main/videos/simulate-lowWater-log.mp4" type="video/mp4">
  Tu navegador no soporta video HTML5. <a href="https://github.com/theinsideshine/fs_logger_demo/raw/main/videos/simulate-lowWater-log.mp4">Descargar MP4</a>
</video>


Estos videos muestran el uso de los comandos por Serial (`rot +1d`, `rot reset`, 
`log lowwater`, `log burst`, etc.) y cómo el logger responde creando nuevos archivos o borrando antiguos.


## Licencia
MIT.
