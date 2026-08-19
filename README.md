💊 Medication Timer — Multi‑Language Reminder for Recurring Doses
8 languages, one reliable medication timer – track your pills, get reminded when it's time to take them, and never miss a dose again.

✨ Features
💊 Add medications – name, dosage, interval (hours), and start time

⏰ Check due doses – see which medications are due now or overdue

✅ Take a dose – mark a medication as taken (resets the timer)

📋 History – view all taken doses with timestamps

💾 Persistent storage – all data saved in a local JSON file

🔔 Console notifications – clearly shows which medications are due

📊 Status overview – list all medications with next dose time

🚀 Quick Start
All implementations share the same CLI interface:

bash
# Add a new medication (name, dosage, interval in hours)
<program> add "Aspirin" "100mg" 8

# List all medications with their next dose time
<program> list

# Take a dose (mark as taken) – use the ID from list
<program> take 1

# Check which medications are due right now
<program> check

# Show history of taken doses
<program> history

# Show help
<program> help
Commands:

add <name> <dosage> <interval> – add a medication

list – show all medications with next dose time

take <id> – mark a dose as taken (resets the timer)

check – show due/overdue medications

history – display recent taken doses

📁 Repository Structure
text
.
├── README.md
├── python/
│   └── medication_timer.py
├── go/
│   └── medication_timer.go
├── javascript/
│   └── medication_timer.js
├── ruby/
│   └── medication_timer.rb
├── php/
│   └── medication_timer.php
├── java/
│   └── MedicationTimer.java
├── csharp/
│   └── MedicationTimer.cs
└── cpp/
    └── medication_timer.cpp
