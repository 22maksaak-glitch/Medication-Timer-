// MedicationTimer.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

class Medication
{
    [JsonPropertyName("id")] public int Id { get; set; }
    [JsonPropertyName("name")] public string Name { get; set; }
    [JsonPropertyName("dosage")] public string Dosage { get; set; }
    [JsonPropertyName("interval_hours")] public double IntervalHours { get; set; }
    [JsonPropertyName("last_taken")] public string LastTaken { get; set; }
    [JsonPropertyName("history")] public List<string> History { get; set; } = new List<string>();

    public bool IsDue()
    {
        var last = DateTime.Parse(LastTaken);
        var next = last.AddHours(IntervalHours);
        return DateTime.Now >= next;
    }

    public DateTime NextDose()
    {
        var last = DateTime.Parse(LastTaken);
        return last.AddHours(IntervalHours);
    }

    public void TakeDose()
    {
        var now = DateTime.Now;
        LastTaken = now.ToString("o");
        History.Add(now.ToString("o"));
    }
}

class TimerApp
{
    private List<Medication> medications = new List<Medication>();
    private readonly string dataFile = "medications.json";
    private readonly JsonSerializerOptions options = new JsonSerializerOptions { WriteIndented = true };

    public TimerApp() { Load(); }

    private void Load()
    {
        if (!File.Exists(dataFile)) return;
        string json = File.ReadAllText(dataFile);
        medications = JsonSerializer.Deserialize<List<Medication>>(json) ?? new List<Medication>();
    }

    private void Save()
    {
        string json = JsonSerializer.Serialize(medications, options);
        File.WriteAllText(dataFile, json);
    }

    public void AddMedication(string name, string dosage, double interval)
    {
        int maxId = medications.Any() ? medications.Max(m => m.Id) : 0;
        var med = new Medication
        {
            Id = maxId + 1,
            Name = name,
            Dosage = dosage,
            IntervalHours = interval,
            LastTaken = DateTime.Now.ToString("o")
        };
        medications.Add(med);
        Save();
        Console.WriteLine($"💊 Added medication: {name} {dosage} every {interval}h (ID: {med.Id})");
    }

    public void List()
    {
        if (!medications.Any()) { Console.WriteLine("No medications."); return; }
        Console.WriteLine("\n📋 Medications:");
        foreach (var m in medications)
        {
            string due = m.IsDue() ? "\u001b[31m🔴 DUE\u001b[0m" :
                        $"\u001b[32m⏳ {m.NextDose():HH:mm}\u001b[0m";
            Console.WriteLine($"  [{m.Id}] {m.Name} {m.Dosage} – {due}");
        }
    }

    public void TakeDose(int id)
    {
        var m = medications.FirstOrDefault(med => med.Id == id);
        if (m == null) { Console.WriteLine($"❌ Medication with ID {id} not found."); return; }
        m.TakeDose();
        Save();
        Console.WriteLine($"✅ Took {m.Name} {m.Dosage} at {DateTime.Now:HH:mm}");
    }

    public void CheckDue()
    {
        var due = medications.Where(m => m.IsDue()).ToList();
        if (!due.Any()) { Console.WriteLine("✅ No medications due right now."); return; }
        Console.WriteLine("\n⏰ Due Medications:");
        foreach (var m in due)
        {
            long overdue = (long)(DateTime.Now - m.NextDose()).TotalMinutes;
            Console.WriteLine($"  [{m.Id}] {m.Name} {m.Dosage} – overdue by {overdue} min");
        }
    }

    public void History()
    {
        if (!medications.Any()) { Console.WriteLine("No medications."); return; }
        Console.WriteLine("\n📜 History (last 5 doses per medication):");
        foreach (var m in medications)
        {
            Console.WriteLine($"  {m.Name}:");
            var recent = m.History.TakeLast(5).ToList();
            if (!recent.Any())
                Console.WriteLine("    (no history)");
            else
                foreach (var ts in recent)
                    Console.WriteLine($"    - {DateTime.Parse(ts):yyyy-MM-dd HH:mm}");
        }
    }

    static void Main(string[] args)
    {
        if (args.Length < 1) { Console.WriteLine("Usage: MedicationTimer <command> [options]"); return; }
        var app = new TimerApp();
        string cmd = args[0];
        switch (cmd)
        {
            case "add":
                if (args.Length < 4) { Console.WriteLine("add <name> <dosage> <interval>"); return; }
                app.AddMedication(args[1], args[2], double.Parse(args[3]));
                break;
            case "list":
                app.List();
                break;
            case "take":
                if (args.Length < 2) { Console.WriteLine("take <id>"); return; }
                app.TakeDose(int.Parse(args[1]));
                break;
            case "check":
                app.CheckDue();
                break;
            case "history":
                app.History();
                break;
            default:
                Console.WriteLine("Unknown command.");
                break;
        }
    }
}
