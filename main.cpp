#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib> // random value generation for the simulated mode
#include <ctime>  // timestamp
#include <fstream>
#include <iomanip> // Manual spacing

using namespace std;

// Forward declaration
class Observer;

// ============== OBSERVER INTERFACE ==============
class Observer {
public:
    virtual void update(float temp, float humidity, float pressure) = 0;
    virtual string getName() const = 0;
    virtual ~Observer() {}
};

// ============== SUBJECT (WEATHER DATA) ==============
class WeatherData {
private:
    float temperature;
    float humidity;
    float pressure;
    vector<Observer*> observers;

    // Historical data storage
    struct Reading {
        float temp;
        float humidity;
        float pressure;
        time_t timestamp;
    };
    vector<Reading> history;
    const int MAX_HISTORY = 10;  // Store last 10 readings

public:
    WeatherData() {
        temperature = 70.0f;
        humidity = 65.0f;
        pressure = 1013.0f;
        srand(time(0));
        loadHistoryFromFile();
    }

    void registerObserver(Observer* o) {
        observers.push_back(o);
        cout << o->getName() << " has been added!\n";
    }

    void notifyObservers() {
        for (Observer* observer : observers) {
            observer->update(temperature, humidity, pressure);
        }
    }

    void setMeasurements(float temp, float humidity, float pressure) {
        this->temperature = temp;
        this->humidity = humidity;
        this->pressure = pressure;

        // Store in history (req 14)
        addToHistory(temp, humidity, pressure);

        measurementsChanged();
    }

    void measurementsChanged() {
        notifyObservers();
        saveHistoryToFile();
    }

    // Simulated mode (req 13)
    void simulateRandomChange() {
        float tempChange = (rand() % 100 - 50) / 10.0f;  // -5.0 to +5.0
        float humChange = (rand() % 40 - 20) / 10.0f;    // -2.0 to +2.0
        float pressChange = (rand() % 60 - 30) / 10.0f;   // -3.0 to +3.0

        float newTemp = temperature + tempChange;
        float newHum = humidity + humChange;
        float newPress = pressure + pressChange;

        // Keep within realistic ranges
        if (newTemp < -50) newTemp = -50;
        if (newTemp > 130) newTemp = 130;
        if (newHum < 0) newHum = 0;
        if (newHum > 100) newHum = 100;
        if (newPress < 950) newPress = 950;
        if (newPress > 1050) newPress = 1050;

        setMeasurements(newTemp, newHum, newPress);
    }

    // Historical data methods (req 14)
    void addToHistory(float temp, float humidity, float pressure) {
        Reading reading;
        reading.temp = temp;
        reading.humidity = humidity;
        reading.pressure = pressure;
        reading.timestamp = time(0);

        history.push_back(reading);
        if (history.size() > MAX_HISTORY) {
            history.erase(history.begin());
        }
    }

    void displayHistory() {
        if (history.empty()) {
            cout << "No historical data available.\n";
            return;
        }

        cout << "\n======= LAST " << history.size() << " READINGS =======\n";
        cout << left << setw(20) << "Time"
             << setw(15) << "Temp (°F)"
             << setw(15) << "Humidity (%)"
             << setw(15) << "Pressure (hPa)" << endl;
        cout << "------------------------------------------------\n";

        for (const Reading& r : history) {
            char* timeStr = ctime(&r.timestamp);
            string timeString(timeStr);
            timeString.pop_back(); // Remove newline

            cout << left << setw(20) << timeString
                 << setw(15) << fixed << setprecision(1) << r.temp
                 << setw(15) << r.humidity
                 << setw(15) << r.pressure << endl;
        }
        cout << "================================================\n";
    }

    void loadHistoryFromFile() {
        ifstream file("weather_history.txt");
        if (!file.is_open()) return;

        Reading r;
        while (file >> r.temp >> r.humidity >> r.pressure >> r.timestamp) {
            history.push_back(r);
            if (history.size() > MAX_HISTORY) {
                history.erase(history.begin());
            }
        }
        file.close();
    }

    void saveHistoryToFile() {
        ofstream file("weather_history.txt");
        for (const Reading& r : history) {
            file << r.temp << " " << r.humidity << " " << r.pressure << " " << r.timestamp << endl;
        }
        file.close();
    }

    float getTemperature() { return temperature; }
    float getHumidity() { return humidity; }
    float getPressure() { return pressure; }
};

// ============== DISPLAYS (OBSERVERS) ==============

// 1. Current Conditions Display
class CurrentConditionsDisplay : public Observer {
private:
    float temp;
    float humidity;
    WeatherData* weatherData;

public:
    CurrentConditionsDisplay(WeatherData* wd) : weatherData(wd) {
        weatherData->registerObserver(this);
    }

    void update(float temp, float humidity, float pressure) override {
        this->temp = temp;
        this->humidity = humidity;
        display();
    }

    void display() {
        float heatIndex = calculateHeatIndex(temp, humidity);
        cout << "\n[Current Conditions]\n";
        cout << "  Temperature: " << fixed << setprecision(1) << temp << "°F\n";
        cout << "  Humidity: " << humidity << "%\n";
        cout << "  Heat Index: " << heatIndex << "°F\n";
    }

    float calculateHeatIndex(float t, float rh) {
        // Simplified heat index formula
        return 0.5f * (t + 61.0f + ((t - 68.0f) * 1.2f) + (rh * 0.094f));
    }

    string getName() const override { return "Current Conditions Display"; }
};

// 2. Statistics Display
class StatisticsDisplay : public Observer {
private:
    vector<float> tempHistory;
    float maxTemp;
    float minTemp;
    float sumTemp;
    int readingCount;

public:
    StatisticsDisplay() {
        maxTemp = -1000;
        minTemp = 1000;
        sumTemp = 0;
        readingCount = 0;
    }

    void update(float temp, float humidity, float pressure) override {
        tempHistory.push_back(temp);
        maxTemp = max(maxTemp, temp);
        minTemp = min(minTemp, temp);
        sumTemp += temp;
        readingCount++;
        display();
    }

    void display() {
        float avgTemp = (readingCount > 0) ? sumTemp / readingCount : 0;

        cout << "\n[Statistics Display]\n";
        cout << "  Readings Count: " << readingCount << endl;
        cout << "  Average Temperature: " << fixed << setprecision(1) << avgTemp << "°F\n";
        cout << "  Max Temperature: " << maxTemp << "°F\n";
        cout << "  Min Temperature: " << minTemp << "°F\n";
    }

    string getName() const override { return "Statistics Display"; }
};

// 3. Forecast Display
class ForecastDisplay : public Observer {
private:
    float lastPressure;
    float currentPressure;

public:
    ForecastDisplay() {
        lastPressure = 1013.0f;
        currentPressure = 1013.0f;
    }

    void update(float temp, float humidity, float pressure) override {
        lastPressure = currentPressure;
        currentPressure = pressure;
        display();
    }

    void display() {
        cout << "\n[Forecast Display]\n";
        cout << "  Forecast: ";

        if (currentPressure > lastPressure) {
            cout << "Improving weather! Expect clearer skies.\n";
        } else if (currentPressure < lastPressure) {
            cout << "Warning: Pressure dropping. Cooler, rainy weather likely.\n";
        } else {
            cout << "More of the same. Weather stable.\n";
        }
        cout << "  Current Pressure: " << fixed << setprecision(1) << currentPressure << " hPa\n";
    }

    string getName() const override { return "Forecast Display"; }
};

// 4. Alert System
class AlertSystem : public Observer {
public:
    void update(float temp, float humidity, float pressure) override {
        bool alertTriggered = false;

        if (temp > 100.0f) {
            cout << "\n ALERT: Extreme heat detected! Temperature: " << temp << "°F\n";
            alertTriggered = true;
        }
        if (temp < 32.0f) {
            cout << "\n ALERT: Freezing conditions! Temperature: " << temp << "°F\n";
            alertTriggered = true;
        }
        if (humidity > 90.0f) {
            cout << "\n️ ALERT: High humidity! Risk of mold: " << humidity << "%\n";
            alertTriggered = true;
        }
        if (humidity < 20.0f) {
            cout << "\n ALERT: Very dry conditions! Humidity: " << humidity << "%\n";
            alertTriggered = true;
        }

        if (!alertTriggered) {
            // Uncomment if you want to see "all clear" messages
            // cout << "\n[Alert System] All conditions normal.\n";
        }
    }

    string getName() const override { return "Alert System"; }
};

// ============== MAIN PROGRAM ==============
int main() {
    cout << "======================================\n";
    cout << "   WEATHER MONITORING SYSTEM\n";
    cout << "   Observer Pattern Demo\n";
    cout << "======================================\n\n";

    WeatherData weatherData;

    // Create displays and register them
    CurrentConditionsDisplay currentDisplay(&weatherData);
    StatisticsDisplay statsDisplay;
    ForecastDisplay forecastDisplay;
    AlertSystem alertDisplay;

    // Register remaining displays
    weatherData.registerObserver(&statsDisplay);
    weatherData.registerObserver(&forecastDisplay);
    weatherData.registerObserver(&alertDisplay);

    int choice;
    bool running = true;
    bool simulationRunning = false;

    while (running) {
        cout << "\n========== MAIN MENU ==========\n";
        cout << "1. Manual data entry\n";
        cout << "2. Simulated mode (auto-update every 2 seconds)\n";
        cout << "3. Stop simulation\n";
        cout << "4. View historical data (last 10 readings)\n";
        cout << "5. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        switch (choice) {
            case 1: {  // Manual data entry
                float temp, humidity, pressure;
                cout << "\nEnter temperature (°F): ";
                cin >> temp;
                cout << "Enter humidity (%): ";
                cin >> humidity;
                cout << "Enter pressure (hPa): ";
                cin >> pressure;

                // Simple validation
                if (temp >= -50 && temp <= 130 &&
                    humidity >= 0 && humidity <= 100 &&
                    pressure >= 950 && pressure <= 1050) {
                    weatherData.setMeasurements(temp, humidity, pressure);
                } else {
                    cout << "Invalid input! Please check ranges.\n";
                }
                break;
            }

            case 2: {  // Simulated mode
                if (!simulationRunning) {
                    simulationRunning = true;
                    cout << "\nStarting simulation mode...\n";
                    cout << "Weather will automatically update every 2 seconds.\n";
                    cout << "Select option 3 to stop simulation.\n\n";

                    // Run 5 simulation cycles for demo (so it doesn't run forever)
                    for (int i = 0; i < 5 && simulationRunning; i++) {
                        cout << "\n--- Update " << (i+1) << " ---\n";
                        weatherData.simulateRandomChange();

                        // Wait 2 seconds (works on most systems)
                        cout << "Waiting 2 seconds...\n";
                        for (int wait = 0; wait < 2; wait++) {
                            if (!simulationRunning) break;
                            #ifdef _WIN32
                                _sleep(1000);
                            #else
                                sleep(1);
                            #endif
                        }
                    }
                    simulationRunning = false;
                    cout << "\nSimulation completed (5 cycles).\n";
                } else {
                    cout << "Simulation is already running!\n";
                }
                break;
            }

            case 3: {  // Stop simulation
                if (simulationRunning) {
                    simulationRunning = false;
                    cout << "Simulation stopped.\n";
                } else {
                    cout << "No simulation is currently running.\n";
                }
                break;
            }

            case 4: {  // View historical data (req 14)
                weatherData.displayHistory();
                break;
            }

            case 5: {  // Exit
                cout << "\nSaving data and exiting...\n";
                weatherData.saveHistoryToFile();
                cout << "Goodbye!\n";
                running = false;
                break;
            }

            default:
                cout << "Invalid choice! Please try again.\n";
        }
    }

    return 0;
}
