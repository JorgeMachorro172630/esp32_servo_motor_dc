# Sistema de Control Climático Automatizado con ESP32

## Descripción
Este proyecto implementa un sistema inteligente de control climático mediante un ESP32 que:
1. **Monitorea condiciones ambientales** usando un sensor DHT11 (temperatura y humedad)
2. **Activa dispositivos automáticamente** según umbrales predefinidos:
   -  **Ventilador (Motor DC)**: Se activa cuando temperatura ≥ 30°C
   -  **Limpiaparabrisas (Servo SG90)**: Activa cuando humedad ≥ 65%
3. **Interfaz web local** para control manual remoto de dispositivos
4. **Sistema de alertas** con buzzer cuando temperatura excede 38°C
5. **Visualización en LCD 16x2** con estado en tiempo real

## Hardware Utilizado
| Componente          | Especificaciones                     | Función                             |
|---------------------|--------------------------------------|-------------------------------------|
| Microcontrolador    | ESP32S (30 pines)                    | Cerebro del sistema                 |
| Sensor ambiental    | DHT11 (4 pines)                      | Mide temperatura y humedad          |
| Motor DC            | 12V con transistor 2N222A           | Ventilador/refrigeración            |
| Servomotor          | SG90                                 | Simula limpiaparabrisas             |
| Pantalla            | LCD 16x2 con interfaz I2C            | Muestra datos y estados             |
| Buzzer              | Pasivo                               | Alerta por alta temperatura         |

## Diagrama de Funcionamiento
```mermaid
graph TD
    A[DHT11] -->|Datos| B(ESP32)
    B --> C{Análisis}
    C -->|Temp ≥ 30°C| D[Activar Motor]
    C -->|Humedad ≥ 65%| E[Activar Servo]
    C -->|Temp ≥ 38°C| F[Activar Alarma]
    B --> G[LCD 16x2]
    B --> H[Servidor Web]
    H --> I[Control Remoto]
