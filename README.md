# Monitor IoT do Depósito

Projeto acadêmico de IoT para monitorar temperatura e umidade em um depósito. O sistema usa um ESP32 e um sensor DHT22, aciona um alarme local e envia dados ao ThingSpeak por MQTT.

## Funcionamento

- Faz leituras do DHT22 a cada 2 segundos.
- Publica temperatura e umidade no ThingSpeak a cada 20 segundos.
- Liga o LED quando a temperatura ultrapassa 28 °C.
- Mantém o alarme ligado até a temperatura cair abaixo de 27 °C.
- Após três leituras inválidas consecutivas do sensor, envia o status `FALHA_SENSOR`.

## Tecnologias e componentes

- ESP32
- Sensor DHT22
- LED 
- MQTT
- ThingSpeak
- Simulação no Wokwi

## Configuração

Configure no código as credenciais MQTT do seu dispositivo ThingSpeak e o ID do canal. Não publique credenciais reais em repositórios públicos.

```cpp
const char* MQTT_CLIENT_ID = "SEU_CLIENT_ID";
const char* MQTT_USUARIO   = "SEU_USERNAME";
const char* MQTT_SENHA     = "SUA_SENHA";
