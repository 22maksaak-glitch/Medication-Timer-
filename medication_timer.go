// medication_timer.go
package main

import (
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"time"
)

type Medication struct {
	ID           int       `json:"id"`
	Name         string    `json:"name"`
	Dosage       string    `json:"dosage"`
	IntervalHours float64  `json:"interval_hours"`
	LastTaken    time.Time `json:"last_taken"`
	History      []time.Time `json:"history"`
}

func (m *Medication) IsDue() bool {
	next := m.LastTaken.Add(time.Duration(m.IntervalHours * float64(time.Hour)))
	return time.Now().After(next) || time.Now().Equal(next)
}

func (m *Medication) NextDose() time.Time {
	return m.LastTaken.Add(time.Duration(m.IntervalHours * float64(time.Hour)))
}

func (m *Medication) TakeDose() {
	now := time.Now()
	m.LastTaken = now
	m.History = append(m.History, now)
}

type TimerApp struct {
	Medications []Medication `json:"medications"`
	File        string
}

func NewTimerApp(file string) *TimerApp {
	app := &TimerApp{File: file}
	app.load()
	return app
}

func (t *TimerApp) load() {
	data, err := os.ReadFile(t.File)
	if err != nil {
		return
	}
	json.Unmarshal(data, t)
}

func (t *TimerApp) save() {
	data, _ := json.MarshalIndent(t, "", "  ")
	os.WriteFile(t.File, data, 0644)
}

func (t *TimerApp) AddMedication(name, dosage string, interval float64) {
	maxID := 0
	for _, m := range t.Medications {
		if m.ID > maxID {
			maxID = m.ID
		}
	}
	med := Medication{
		ID:            maxID + 1,
		Name:          name,
		Dosage:        dosage,
		IntervalHours: interval,
		LastTaken:     time.Now(),
		History:       []time.Time{},
	}
	t.Medications = append(t.Medications, med)
	t.save()
	fmt.Printf("💊 Added medication: %s %s every %.1fh (ID: %d)\n", name, dosage, interval, med.ID)
}

func (t *TimerApp) List() {
	if len(t.Medications) == 0 {
		fmt.Println("No medications.")
		return
	}
	fmt.Println("\n📋 Medications:")
	for _, m := range t.Medications {
		due := "🔴 DUE"
		if !m.IsDue() {
			due = fmt.Sprintf("⏳ %02d:%02d", m.NextDose().Hour(), m.NextDose().Minute())
		}
		fmt.Printf("  [%d] %s %s – %s\n", m.ID, m.Name, m.Dosage, due)
	}
}

func (t *TimerApp) TakeDose(id int) {
	for i, m := range t.Medications {
		if m.ID == id {
			t.Medications[i].TakeDose()
			t.save()
			fmt.Printf("✅ Took %s %s at %s\n", m.Name, m.Dosage, time.Now().Format("15:04"))
			return
		}
	}
	fmt.Printf("❌ Medication with ID %d not found.\n", id)
}

func (t *TimerApp) CheckDue() {
	due := []Medication{}
	for _, m := range t.Medications {
		if m.IsDue() {
			due = append(due, m)
		}
	}
	if len(due) == 0 {
		fmt.Println("✅ No medications due right now.")
		return
	}
	fmt.Println("\n⏰ Due Medications:")
	for _, m := range due {
		overdue := int(time.Now().Sub(m.NextDose()).Minutes())
		fmt.Printf("  [%d] %s %s – overdue by %d min\n", m.ID, m.Name, m.Dosage, overdue)
	}
}

func (t *TimerApp) History() {
	if len(t.Medications) == 0 {
		fmt.Println("No medications.")
		return
	}
	fmt.Println("\n📜 History (last 5 doses per medication):")
	for _, m := range t.Medications {
		fmt.Printf("  %s:\n", m.Name)
		recent := m.History
		if len(recent) > 5 {
			recent = recent[len(recent)-5:]
		}
		if len(recent) == 0 {
			fmt.Println("    (no history)")
		} else {
			for _, ts := range recent {
				fmt.Printf("    - %s\n", ts.Format("2006-01-02 15:04"))
			}
		}
	}
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: medication_timer <command> [options]")
		return
	}
	app := NewTimerApp("medications.json")
	cmd := os.Args[1]
	switch cmd {
	case "add":
		if len(os.Args) < 5 {
			fmt.Println("add <name> <dosage> <interval_hours>")
			return
		}
		name := os.Args[2]
		dosage := os.Args[3]
		interval, _ := strconv.ParseFloat(os.Args[4], 64)
		app.AddMedication(name, dosage, interval)
	case "list":
		app.List()
	case "take":
		if len(os.Args) < 3 {
			fmt.Println("take <id>")
			return
		}
		id, _ := strconv.Atoi(os.Args[2])
		app.TakeDose(id)
	case "check":
		app.CheckDue()
	case "history":
		app.History()
	default:
		fmt.Println("Unknown command.")
	}
}
