#include <iostream>
#include <cstring>
#include <cassert>

// Definiera din batteristatusstruktur
struct battery_status_parameters {
    float battery_level;     
    bool is_charging;        
    char status_message[32]; 
    float voltage;           

    // Konstruktor
    battery_status_parameters()
        : battery_level(0.0f), is_charging(false), voltage(0.0f) {
        strcpy(status_message, "Unknown status");
    }
};

// Definiera din meddelande struktur
struct message {
    uint8_t data[64];  // Simulerad rådata
};

// Din unpack-funktion (som du redan har)
inline void unpack_battery_status(message &raw_msg, battery_status_parameters &status) {
    uint16_t offset = 0;
    int32_t voltage_raw;
    uint8_t charging_status;

    // Läs batterivoltage (i millivolt, konvertera till volt)
    memcpy((void *)&voltage_raw, (void *)&raw_msg.data[offset], sizeof(int32_t));
    status.voltage = static_cast<float>(voltage_raw) / 1000.0f;  // Omvandling till volt
    offset += sizeof(int32_t);  // Uppdatera offset efter att ha läst voltage

    // Läs batteriprocent (float)
    memcpy((void *)&status.battery_level, (void *)&raw_msg.data[offset], sizeof(float));
    offset += sizeof(float);  // Uppdatera offset efter att ha läst procent

    // Läs laddstatus (t.ex. 0 för ej laddning, 1 för laddning)
    memcpy((void *)&charging_status, (void *)&raw_msg.data[offset], sizeof(uint8_t));
    status.is_charging = (charging_status != 0);  // Om 1, så laddar det
    offset += sizeof(uint8_t);  // Uppdatera offset efter att ha läst laddstatus

    // Sätt status meddelande
    if (status.is_charging) {
        strcpy(status.status_message, "Charging");
    } else if (status.battery_level < 20.0f) {
        strcpy(status.status_message, "Low battery");
    } else {
        strcpy(status.status_message, "Normal");
    }
}

int main() {
    // Skapa ett meddelande som ska simuleras
    message raw_msg;

    // Fyll med testdata: voltage (millivolt), batterinivå (float), charging status (0 eller 1)
    int32_t voltage = 3700;  // 3.7V (i millivolt)
    float battery_level = 45.5f;  // 45.5%
    uint8_t charging_status = 1;  // Laddas (1)

    // Kopiera data till vårt meddelande
    memcpy(raw_msg.data, &voltage, sizeof(int32_t));
    memcpy(raw_msg.data + sizeof(int32_t), &battery_level, sizeof(float));
    memcpy(raw_msg.data + sizeof(int32_t) + sizeof(float), &charging_status, sizeof(uint8_t));

    // Skapa en instans av batteristatus
    battery_status_parameters status;

    // Anropa unpack-funktionen för att extrahera data
    unpack_battery_status(raw_msg, status);

    // Skriv ut resultatet för att kontrollera
    std::cout << "Battery Level: " << status.battery_level << "%" << std::endl;
    std::cout << "Charging: " << (status.is_charging ? "Yes" : "No") << std::endl;
    std::cout << "Status Message: " << status.status_message << std::endl;
    std::cout << "Voltage: " << status.voltage << "V" << std::endl;

    // Kontrollera om det var korrekt
    assert(status.battery_level == 45.5f);  // Kontrollera att batterinivån stämmer
    assert(status.is_charging == true);    // Kontrollera att laddstatus stämmer
    assert(strcmp(status.status_message, "Charging") == 0);  // Kontrollera statusmeddelandet

    return 0;
}
