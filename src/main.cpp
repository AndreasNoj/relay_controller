// Signal K application template file.
//
// This application demonstrates core SensESP concepts using four individual
// switches. Each pushbutton toggles its corresponding relay, publishes its
// state to a unique SignalK path, and also drives a dedicated status LED.

#include <Wire.h>

#include <memory>

#include "sensesp.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/digital_output.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/signalk/signalk_put_request_listener.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/transforms/repeat_report.h"
#include "sensesp/transforms/press_repeater.h"
#include "sensesp/ui/config_item.h"
#include "sensesp_app_builder.h"

// I2C pins (if needed for other sensors)
#define I2C_SDA 21
#define I2C_SCL 22

using namespace sensesp;
using namespace reactesp;

void setup() {
  SetupLogging(ESP_LOG_DEBUG);
  Wire.begin(I2C_SDA, I2C_SCL);

  // Build the SensESP application.
  SensESPAppBuilder builder;
  sensesp_app = (&builder)
                    ->set_hostname("Light-Inside-Relays")
                    ->set_wifi_client("Obelix", "obelix2idefix")
                    ->enable_uptime_sensor()
                    ->get_app();

  // GPIO configuration:
  // Status LED pins for each relay.
  const int led1StatusPin = 12;
  const int led2StatusPin = 13;
  const int led3StatusPin = 14;
  const int led4StatusPin = 15;
  // Pushbutton pins for the four switches.
  const int button1Pin = 16;
  const int button2Pin = 17;
  const int button3Pin = 18;
  const int button4Pin = 19;
  // Relay pins.
  const int relay1Pin = 32;  // Relay 1
  const int relay2Pin = 33;  // Relay 2
  const int relay3Pin = 25;  // Relay 3
  const int relay4Pin = 26;  // Relay 4

  // Define SignalK paths for each relay.
  const char* sk_path_relay_1 = "electrical.switches.light.cabin.state";
  const char* sk_path_relay_2 = "electrical.switches.light.port.state";
  const char* sk_path_relay_3 = "electrical.switches.light.starboard.state";
  const char* sk_path_relay_4 = "electrical.switches.light.engine.state";


  Serial.println(F("Starting 4 individual relay switches with status LEDs..."));

  // Create DigitalInputChange objects for each pushbutton.
  auto* button1 = new DigitalInputChange(button1Pin, INPUT_PULLUP, CHANGE);
  auto* button2 = new DigitalInputChange(button2Pin, INPUT_PULLUP, CHANGE);
  auto* button3 = new DigitalInputChange(button3Pin, INPUT_PULLUP, CHANGE);
  auto* button4 = new DigitalInputChange(button4Pin, INPUT_PULLUP, CHANGE);

  // Create DigitalOutput objects for each relay.
  auto* relay1 = new DigitalOutput(relay1Pin);
  auto* relay2 = new DigitalOutput(relay2Pin);
  auto* relay3 = new DigitalOutput(relay3Pin);
  auto* relay4 = new DigitalOutput(relay4Pin);

  // Initialize all relays to off (0).
  relay1->set(1);
  relay2->set(1);
  relay3->set(1);
  relay4->set(1);

  // Create DigitalOutput objects for the status LEDs.
  auto* led1 = new DigitalOutput(led1StatusPin);
  auto* led2 = new DigitalOutput(led2StatusPin);
  auto* led3 = new DigitalOutput(led3StatusPin);
  auto* led4 = new DigitalOutput(led4StatusPin);

  // For each switch, connect its button to a lambda that toggles its relay.
  // When the relay is updated, the change is propagated (via a chain
  // connection) to both the SignalK output and the LED.
  button1->connect_to(new LambdaConsumer<bool>([relay1](bool isPressed) {
    if (isPressed) {
      bool new_state = !relay1->get();
      relay1->set(new_state);
      debugD("Relay1 toggled to: %d", new_state);
    }
  }));

  button2->connect_to(new LambdaConsumer<bool>([relay2](bool isPressed) {
    if (isPressed) {
      bool new_state = !relay2->get();
      relay2->set(new_state);
      debugD("Relay2 toggled to: %d", new_state);
    }
  }));

  button3->connect_to(new LambdaConsumer<bool>([relay3](bool isPressed) {
    if (isPressed) {
      bool new_state = !relay3->get();
      relay3->set(new_state);
      debugD("Relay3 toggled to: %d", new_state);
    }
  }));

  button4->connect_to(new LambdaConsumer<bool>([relay4](bool isPressed) {
    if (isPressed) {
      bool new_state = !relay4->get();
      relay4->set(new_state);
      debugD("Relay4 toggled to: %d", new_state);
    }
  }));

  // Configure SignalK output for Relay 1.
  auto relay1_metadata = std::make_shared<SKMetadata>("", "Relay1 state");
  auto relay1_sk_output = std::make_shared<SKOutput<bool>>(
      sk_path_relay_1,             // SignalK path
      "/Electrical/Relay1/Value",  // Configuration path
      relay1_metadata);
  ConfigItem(relay1_sk_output)
      ->set_title("Relay 1 SK Output Path")
      ->set_sort_order(200);
  // Connect relay1 to both its SignalK output and its status LED.
  relay1->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(relay1_sk_output);
  relay1->connect_to(
      new LambdaConsumer<bool>([led1](bool state) { led1->set(state); }));

  // Add a SignalK PUT listener for Relay 1 using SKPutRequestListener.
  auto relay1_put_listener = new SKPutRequestListener<bool>(sk_path_relay_1);
  relay1_put_listener->connect_to(
      new LambdaConsumer<bool>([relay1, led1](bool new_state) {
        relay1->set(new_state);
        led1->set(new_state);
        debugD("Relay1 updated from SK PUT to: %d", new_state);
      }));

  // Configure SignalK output for Relay 2.
  auto relay2_metadata = std::make_shared<SKMetadata>("", "Relay2 state");
  auto relay2_sk_output = std::make_shared<SKOutput<bool>>(
      sk_path_relay_2, "/Electrical/Relay2/Value", relay2_metadata);
  ConfigItem(relay2_sk_output)
      ->set_title("Relay 2 SK Output Path")
      ->set_sort_order(210);
  relay2->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(relay2_sk_output);
  relay2->connect_to(
      new LambdaConsumer<bool>([led2](bool state) { led2->set(state); }));

  // Add a SignalK PUT listener for Relay 2.
  auto relay2_put_listener = new SKPutRequestListener<bool>(sk_path_relay_2);
  relay2_put_listener->connect_to(
      new LambdaConsumer<bool>([relay2, led2](bool new_state) {
        relay2->set(new_state);
        led2->set(new_state);
        debugD("Relay2 updated from SK PUT to: %d", new_state);
      }));

  // Configure SignalK output for Relay 3.
  auto relay3_metadata = std::make_shared<SKMetadata>("", "Relay3 state");
  auto relay3_sk_output = std::make_shared<SKOutput<bool>>(
      sk_path_relay_3, "/Electrical/Relay3/Value", relay3_metadata);
  ConfigItem(relay3_sk_output)
      ->set_title("Relay 3 SK Output Path")
      ->set_sort_order(220);
  relay3->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(relay3_sk_output);
  relay3->connect_to(
      new LambdaConsumer<bool>([led3](bool state) { led3->set(state); }));

  // Add a SignalK PUT listener for Relay 3.
  auto relay3_put_listener = new SKPutRequestListener<bool>(sk_path_relay_3);
  relay3_put_listener->connect_to(
      new LambdaConsumer<bool>([relay3, led3](bool new_state) {
        relay3->set(new_state);
        led3->set(new_state);
        debugD("Relay3 updated from SK PUT to: %d", new_state);
      }));

  // Configure SignalK output for Relay 4.
  auto relay4_metadata = std::make_shared<SKMetadata>("", "Relay4 state");
  auto relay4_sk_output = std::make_shared<SKOutput<bool>>(
      sk_path_relay_4, "/Electrical/Relay4/Value", relay4_metadata);
  ConfigItem(relay4_sk_output)
      ->set_title("Relay 4 SK Output Path")
      ->set_sort_order(230);
  relay4->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(relay4_sk_output);
  relay4->connect_to(
      new LambdaConsumer<bool>([led4](bool state) { led4->set(state); }));

  // Add a SignalK PUT listener for Relay 4.
  auto relay4_put_listener = new SKPutRequestListener<bool>(sk_path_relay_4);
  relay4_put_listener->connect_to(
      new LambdaConsumer<bool>([relay4, led4](bool new_state) {
        relay4->set(new_state);
        led4->set(new_state);
        debugD("Relay4 updated from SK PUT to: %d", new_state);
      }));
}

void loop() { event_loop()->tick(); }