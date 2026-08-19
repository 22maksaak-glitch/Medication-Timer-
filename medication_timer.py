# medication_timer.py
import sys, os, json, argparse
from datetime import datetime, timedelta
try:
    from colorama import init, Fore, Style
    init()
    COLORS = True
except ImportError:
    COLORS = False
    Fore = Style = type('', (), {'RESET_ALL':'', 'GREEN':'', 'RED':'', 'YELLOW':'', 'CYAN':''})()

DATA_FILE = "medications.json"

class Medication:
    def __init__(self, name, dosage, interval_hours, last_taken=None, med_id=None):
        self.id = med_id or 0
        self.name = name
        self.dosage = dosage
        self.interval_hours = interval_hours
        self.last_taken = last_taken or datetime.now().isoformat()
        self.history = []  # list of ISO timestamps

    def is_due(self):
        last = datetime.fromisoformat(self.last_taken)
        next_dose = last + timedelta(hours=self.interval_hours)
        return datetime.now() >= next_dose

    def next_dose_time(self):
        last = datetime.fromisoformat(self.last_taken)
        return last + timedelta(hours=self.interval_hours)

    def take_dose(self):
        now = datetime.now()
        self.last_taken = now.isoformat()
        self.history.append(now.isoformat())

    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "dosage": self.dosage,
            "interval_hours": self.interval_hours,
            "last_taken": self.last_taken,
            "history": self.history
        }

    @classmethod
    def from_dict(cls, data):
        m = cls(data["name"], data["dosage"], data["interval_hours"],
                data["last_taken"], data["id"])
        m.history = data.get("history", [])
        return m

class TimerApp:
    def __init__(self):
        self.medications = []
        self.load()

    def load(self):
        if os.path.exists(DATA_FILE):
            with open(DATA_FILE, "r") as f:
                data = json.load(f)
                self.medications = [Medication.from_dict(m) for m in data]

    def save(self):
        with open(DATA_FILE, "w") as f:
            json.dump([m.to_dict() for m in self.medications], f, indent=2)

    def add_medication(self, name, dosage, interval):
        med_id = max([m.id for m in self.medications] + [0]) + 1
        m = Medication(name, dosage, interval, med_id=med_id)
        self.medications.append(m)
        self.save()
        print(f"💊 Added medication: {name} {dosage} every {interval}h (ID: {med_id})")

    def list_medications(self):
        if not self.medications:
            print("No medications.")
            return
        print("\n📋 Medications:")
        for m in self.medications:
            due = "🔴 DUE" if m.is_due() else f"⏳ {m.next_dose_time().strftime('%H:%M')}"
            status_color = Fore.RED if m.is_due() else Fore.GREEN
            if COLORS:
                print(f"  [{m.id}] {m.name} {m.dosage} – {status_color}{due}{Style.RESET_ALL}")
            else:
                print(f"  [{m.id}] {m.name} {m.dosage} – {due}")

    def take_dose(self, med_id):
        for m in self.medications:
            if m.id == med_id:
                m.take_dose()
                self.save()
                print(f"✅ Took {m.name} {m.dosage} at {datetime.now().strftime('%H:%M')}")
                return
        print(f"❌ Medication with ID {med_id} not found.")

    def check_due(self):
        due_list = [m for m in self.medications if m.is_due()]
        if not due_list:
            print("✅ No medications due right now.")
            return
        print("\n⏰ Due Medications:")
        for m in due_list:
            print(f"  [{m.id}] {m.name} {m.dosage} – overdue by {int((datetime.now() - m.next_dose_time()).total_seconds() / 60)} min")

    def history(self):
        if not self.medications:
            print("No medications.")
            return
        print("\n📜 History (last 5 doses per medication):")
        for m in self.medications:
            recent = m.history[-5:] if m.history else []
            print(f"  {m.name}:")
            if recent:
                for ts in recent:
                    dt = datetime.fromisoformat(ts)
                    print(f"    - {dt.strftime('%Y-%m-%d %H:%M')}")
            else:
                print("    (no history)")

def main():
    parser = argparse.ArgumentParser(description="Medication Timer")
    subparsers = parser.add_subparsers(dest="cmd", required=True)

    add_parser = subparsers.add_parser("add")
    add_parser.add_argument("name")
    add_parser.add_argument("dosage")
    add_parser.add_argument("interval", type=float)

    subparsers.add_parser("list")
    take_parser = subparsers.add_parser("take")
    take_parser.add_argument("id", type=int)
    subparsers.add_parser("check")
    subparsers.add_parser("history")

    args = parser.parse_args()
    app = TimerApp()

    if args.cmd == "add":
        app.add_medication(args.name, args.dosage, args.interval)
    elif args.cmd == "list":
        app.list_medications()
    elif args.cmd == "take":
        app.take_dose(args.id)
    elif args.cmd == "check":
        app.check_due()
    elif args.cmd == "history":
        app.history()

if __name__ == "__main__":
    main()
