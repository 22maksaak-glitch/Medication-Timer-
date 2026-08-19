// medication_timer.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct Medication {
    int id;
    string name;
    string dosage;
    double intervalHours;
    time_t lastTaken;
    vector<time_t> history;

    bool isDue() const {
        time_t next = lastTaken + (time_t)(intervalHours * 3600);
        return time(nullptr) >= next;
    }

    time_t nextDose() const {
        return lastTaken + (time_t)(intervalHours * 3600);
    }

    void takeDose() {
        time_t now = time(nullptr);
        lastTaken = now;
        history.push_back(now);
    }
};

class TimerApp {
private:
    vector<Medication> medications;
    string dataFile = "medications.json";

    void load() {
        ifstream f(dataFile);
        if (!f.is_open()) return;
        json j;
        f >> j;
        for (auto& item : j) {
            Medication m;
            m.id = item["id"];
            m.name = item["name"];
            m.dosage = item["dosage"];
            m.intervalHours = item["interval_hours"];
            string lastStr = item["last_taken"];
            // parse ISO time (simple)
            struct tm tm = {};
            strptime(lastStr.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
            m.lastTaken = mktime(&tm);
            for (auto& h : item["history"]) {
                string hs = h;
                struct tm tmh = {};
                strptime(hs.c_str(), "%Y-%m-%dT%H:%M:%S", &tmh);
                m.history.push_back(mktime(&tmh));
            }
            medications.push_back(m);
        }
    }

    void save() {
        json j = json::array();
        for (auto& m : medications) {
            char buf[30];
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", localtime(&m.lastTaken));
            string lastStr(buf);
            vector<string> histStr;
            for (time_t t : m.history) {
                char hbuf[30];
                strftime(hbuf, sizeof(hbuf), "%Y-%m-%dT%H:%M:%S", localtime(&t));
                histStr.push_back(string(hbuf));
            }
            j.push_back({
                {"id", m.id},
                {"name", m.name},
                {"dosage", m.dosage},
                {"interval_hours", m.intervalHours},
                {"last_taken", lastStr},
                {"history", histStr}
            });
        }
        ofstream f(dataFile);
        f << setw(2) << j << endl;
    }

public:
    TimerApp() { load(); }

    void addMedication(const string& name, const string& dosage, double interval) {
        int maxId = 0;
        for (auto& m : medications) if (m.id > maxId) maxId = m.id;
        Medication m;
        m.id = maxId + 1;
        m.name = name;
        m.dosage = dosage;
        m.intervalHours = interval;
        m.lastTaken = time(nullptr);
        medications.push_back(m);
        save();
        cout << "💊 Added medication: " << name << " " << dosage << " every " << interval << "h (ID: " << m.id << ")\n";
    }

    void list() {
        if (medications.empty()) { cout << "No medications.\n"; return; }
        cout << "\n📋 Medications:\n";
        for (auto& m : medications) {
            string due;
            if (m.isDue()) due = "\033[31m🔴 DUE\033[0m";
            else {
                char buf[10];
                time_t next = m.nextDose();
                strftime(buf, sizeof(buf), "%H:%M", localtime(&next));
                due = string("\033[32m⏳ ") + buf + "\033[0m";
            }
            cout << "  [" << m.id << "] " << m.name << " " << m.dosage << " – " << due << "\n";
        }
    }

    void takeDose(int id) {
        for (auto& m : medications) {
            if (m.id == id) {
                m.takeDose();
                save();
                char buf[10];
                time_t now = time(nullptr);
                strftime(buf, sizeof(buf), "%H:%M", localtime(&now));
                cout << "✅ Took " << m.name << " " << m.dosage << " at " << buf << "\n";
                return;
            }
        }
        cout << "❌ Medication with ID " << id << " not found.\n";
    }

    void checkDue() {
        vector<Medication*> due;
        for (auto& m : medications) if (m.isDue()) due.push_back(&m);
        if (due.empty()) { cout << "✅ No medications due right now.\n"; return; }
        cout << "\n⏰ Due Medications:\n";
        for (auto* m : due) {
            long overdue = (time(nullptr) - m->nextDose()) / 60;
            cout << "  [" << m->id << "] " << m->name << " " << m->dosage << " – overdue by " << overdue << " min\n";
        }
    }

    void history() {
        if (medications.empty()) { cout << "No medications.\n"; return; }
        cout << "\n📜 History (last 5 doses per medication):\n";
        for (auto& m : medications) {
            cout << "  " << m.name << ":\n";
            vector<time_t> recent = m.history;
            if (recent.size() > 5) recent = vector<time_t>(recent.end()-5, recent.end());
            if (recent.empty()) {
                cout << "    (no history)\n";
            } else {
                for (time_t t : recent) {
                    char buf[20];
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
                    cout << "    - " << buf << "\n";
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: medication_timer <command> [options]\n";
        return 1;
    }
    TimerApp app;
    string cmd = argv[1];
    if (cmd == "add") {
        if (argc < 5) { cerr << "add <name> <dosage> <interval>\n"; return 1; }
        app.addMedication(argv[2], argv[3], stod(argv[4]));
    } else if (cmd == "list") {
        app.list();
    } else if (cmd == "take") {
        if (argc < 3) { cerr << "take <id>\n"; return 1; }
        app.takeDose(stoi(argv[2]));
    } else if (cmd == "check") {
        app.checkDue();
    } else if (cmd == "history") {
        app.history();
    } else {
        cerr << "Unknown command.\n";
    }
    return 0;
}
