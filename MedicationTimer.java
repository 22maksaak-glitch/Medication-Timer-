// MedicationTimer.java
import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.time.format.*;
import java.util.*;
import com.google.gson.*;

class Medication {
    int id;
    String name;
    String dosage;
    double intervalHours;
    String lastTaken;
    List<String> history;

    public Medication() {}

    public Medication(int id, String name, String dosage, double intervalHours) {
        this.id = id;
        this.name = name;
        this.dosage = dosage;
        this.intervalHours = intervalHours;
        this.lastTaken = Instant.now().toString();
        this.history = new ArrayList<>();
    }

    public boolean isDue() {
        Instant last = Instant.parse(lastTaken);
        Instant next = last.plusSeconds((long)(intervalHours * 3600));
        return Instant.now().compareTo(next) >= 0;
    }

    public Instant nextDose() {
        Instant last = Instant.parse(lastTaken);
        return last.plusSeconds((long)(intervalHours * 3600));
    }

    public void takeDose() {
        Instant now = Instant.now();
        lastTaken = now.toString();
        history.add(now.toString());
    }
}

class TimerApp {
    private List<Medication> medications = new ArrayList<>();
    private final String dataFile = "medications.json";
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    public TimerApp() { load(); }

    private void load() {
        try {
            Path path = Paths.get(dataFile);
            if (Files.exists(path)) {
                String json = new String(Files.readAllBytes(path));
                Medication[] arr = gson.fromJson(json, Medication[].class);
                medications = Arrays.asList(arr);
            }
        } catch (Exception e) {}
    }

    private void save() {
        try {
            Files.write(Paths.get(dataFile), gson.toJson(medications).getBytes());
        } catch (Exception e) {}
    }

    public void addMedication(String name, String dosage, double interval) {
        int maxId = medications.stream().mapToInt(m -> m.id).max().orElse(0);
        Medication m = new Medication(maxId + 1, name, dosage, interval);
        medications.add(m);
        save();
        System.out.printf("💊 Added medication: %s %s every %.1fh (ID: %d)%n", name, dosage, interval, m.id);
    }

    public void list() {
        if (medications.isEmpty()) {
            System.out.println("No medications.");
            return;
        }
        System.out.println("\n📋 Medications:");
        for (Medication m : medications) {
            String due = m.isDue() ? "\u001B[31m🔴 DUE\u001B[0m" :
                    "\u001B[32m⏳ " + LocalDateTime.ofInstant(m.nextDose(), ZoneId.systemDefault()).format(DateTimeFormatter.ofPattern("HH:mm")) + "\u001B[0m";
            System.out.printf("  [%d] %s %s – %s%n", m.id, m.name, m.dosage, due);
        }
    }

    public void takeDose(int id) {
        for (Medication m : medications) {
            if (m.id == id) {
                m.takeDose();
                save();
                System.out.printf("✅ Took %s %s at %s%n", m.name, m.dosage,
                        LocalDateTime.now().format(DateTimeFormatter.ofPattern("HH:mm")));
                return;
            }
        }
        System.out.printf("❌ Medication with ID %d not found.%n", id);
    }

    public void checkDue() {
        List<Medication> due = new ArrayList<>();
        for (Medication m : medications) if (m.isDue()) due.add(m);
        if (due.isEmpty()) {
            System.out.println("✅ No medications due right now.");
            return;
        }
        System.out.println("\n⏰ Due Medications:");
        for (Medication m : due) {
            long overdue = (Instant.now().getEpochSecond() - m.nextDose().getEpochSecond()) / 60;
            System.out.printf("  [%d] %s %s – overdue by %d min%n", m.id, m.name, m.dosage, overdue);
        }
    }

    public void history() {
        if (medications.isEmpty()) {
            System.out.println("No medications.");
            return;
        }
        System.out.println("\n📜 History (last 5 doses per medication):");
        for (Medication m : medications) {
            System.out.printf("  %s:%n", m.name);
            List<String> recent = m.history;
            if (recent.size() > 5) recent = recent.subList(recent.size() - 5, recent.size());
            if (recent.isEmpty()) {
                System.out.println("    (no history)");
            } else {
                for (String ts : recent) {
                    Instant instant = Instant.parse(ts);
                    System.out.printf("    - %s%n",
                            LocalDateTime.ofInstant(instant, ZoneId.systemDefault()).format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm")));
                }
            }
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.out.println("Usage: MedicationTimer <command> [options]");
            return;
        }
        TimerApp app = new TimerApp();
        String cmd = args[0];
        switch (cmd) {
            case "add":
                if (args.length < 4) { System.out.println("add <name> <dosage> <interval>"); return; }
                app.addMedication(args[1], args[2], Double.parseDouble(args[3]));
                break;
            case "list":
                app.list();
                break;
            case "take":
                if (args.length < 2) { System.out.println("take <id>"); return; }
                app.takeDose(Integer.parseInt(args[1]));
                break;
            case "check":
                app.checkDue();
                break;
            case "history":
                app.history();
                break;
            default:
                System.out.println("Unknown command.");
        }
    }
}
