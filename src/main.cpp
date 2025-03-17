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
  // Define the number of remote channels.
  const int num_relays = 4;

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

  int buttonPins[num_relays]   = {16, 17, 18, 19};
  int statusLEDPins[num_relays] = {12, 13, 14, 15};
  int relayPins[num_relays]     = {32, 33, 25, 26};

  // Define SignalK paths for each relay.
  const char* sk_path_relay_1 = "electrical.switches.light.cabin.state";
  const char* sk_path_relay_2 = "electrical.switches.light.port.state";
  const char* sk_path_relay_3 = "electrical.switches.light.starboard.state";
  const char* sk_path_relay_4 = "electrical.switches.light.engine.state";

  const char* default_sk_paths[num_relays] = {
      "electrical.switches.light.cabin.state",
      "electrical.switches.light.port.state",
      "electrical.switches.light.starboard.state",
      "electrical.switches.light.engine.state"};

  Serial.println(F("Starting 4 individual relay switches with status LEDs..."));

  for (int i = 0 ; i < num_relays; i++){
    int relayIndex = i;
    auto* button = new DigitalInputChange(buttonPins[relayIndex], INPUT_PULLUP, CHANGE);
    auto* relay = new DigitalOutput(relayPins[relayIndex]);
    auto led = new DigitalOutput(statusLEDPins[relayIndex]);
    
    relay->set(1);
    
    auto* debouncer = new Debounce<bool>(50);
    button->connect_to(debouncer);

    button->connect_to(new LambdaConsumer<bool>([relay](bool isPressed) {
      if (isPressed) {
        bool new_state = !relay->get();
        relay->set(new_state);
        debugD("Relay %d toggled to: %d", new_state);
      }
    }));

    std::string configPath = "/Remote/Control/Relay" + std::to_string(relayIndex + 1) + "/Value";
    std::string sk_output_title = "Relay " + std::to_string(relayIndex + 1) + " SK Output Path";
    std::string sk_metadata_title = "Remote control relay state for relay " + std::to_string(relayIndex + 1);
    const char* sk_path = default_sk_paths[relayIndex];

    auto metadata = std::make_shared<SKMetadata>("", sk_metadata_title.c_str());
    // Create the SKOutput for this relay channel.
    auto* sk_output = new SKOutput<bool>(
      sk_path, 
      configPath.c_str(), 
      metadata);
    
    // Wrap the SKOutput in a ConfigItem so that its SK path is configurable.
    ConfigItem(sk_output)
          ->set_title(sk_output_title.c_str())
          ->set_sort_order(100 + relayIndex);
          

  // Connect relay1 to both its SignalK output and its status LED.
  relay->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(sk_output);
  relay->connect_to(
      new LambdaConsumer<bool>([led](bool state) { led->set(state); }));

  // Add a SignalK PUT listener for Relay 1 using SKPutRequestListener.
  auto relay_put_listener = new SKPutRequestListener<bool>(sk_path);
  relay_put_listener->connect_to(
      new LambdaConsumer<bool>([relay, led](bool new_state) {
        relay->set(new_state);
        led->set(new_state);
        debugD("Relay1 updated from SK PUT to: %d", new_state);
      }));
  }

  
}

void loop() { event_loop()->tick(); }