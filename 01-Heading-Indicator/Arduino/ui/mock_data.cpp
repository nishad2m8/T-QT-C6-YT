#include "mock_data.h"
#include "ui.h"
#include <Arduino.h>  // For Serial communication

// Define the directions array
const char* directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

// Function to calculate compass direction based on degree
const char* getDirectionFromDegree(int degree) {
    if (degree >= 0 && degree < 45) return directions[0];   // N
    if (degree >= 45 && degree < 90) return directions[1];   // NE
    if (degree >= 90 && degree < 135) return directions[2];  // E
    if (degree >= 135 && degree < 180) return directions[3]; // SE
    if (degree >= 180 && degree < 225) return directions[4]; // S
    if (degree >= 225 && degree < 270) return directions[5]; // SW
    if (degree >= 270 && degree < 315) return directions[6]; // W
    if (degree >= 315 && degree <= 360) return directions[7]; // NW
    return directions[0];  // Default to North if something is wrong
}

// Function to apply custom animation to the roller
void applyRollerAnimation(lv_obj_t * roller, int directionIndex) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_time(&anim, 500);   // Set the animation duration to 500ms
    lv_roller_set_selected(roller, directionIndex, LV_ANIM_ON);  // Use animation
}

// Mock function to update UI with compass and altitude data
void updateMockData() {
    static int degree = 0;  // Starting at 0 degrees (North)
    static unsigned long lastUpdate = 0;  // Track time of last update
    const unsigned long updateInterval = 500;  // Update every 500 milliseconds

    // Only update the degree and altitude every 'updateInterval' milliseconds
    if (millis() - lastUpdate >= updateInterval) {
        lastUpdate = millis();

        // Update degree (360 degrees compass rotation)
        degree = (degree + 10) % 360;  // Increment by 45 degrees for simulation

        // Get corresponding direction from degree
        int directionIndex = degree / 45;

        // Apply smooth animation to the rollers
        applyRollerAnimation(ui_Roller_compass1, directionIndex);
        applyRollerAnimation(ui_Roller_compass3, directionIndex);

        // Update the degree label without the degree symbol
        lv_label_set_text_fmt(ui_Label_degree, "%d", degree);  // Set degree value

        // Update the altitude label without the 'Altitude:' label, only showing the value
        static int mock_altitude = 1500;  // starting altitude in meters
        mock_altitude += (degree % 90 == 0) ? 10 : -5;  // Mocking altitude change
        lv_label_set_text_fmt(ui_Label_altitude, "%d", mock_altitude);  // Only show the altitude value

        // Debugging in Serial Monitor (Optional)
        Serial.printf("Degree: %d, Altitude: %d\n", degree, mock_altitude);
    }
}
