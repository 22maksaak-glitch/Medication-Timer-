# medication_timer.php
<?php
$dataFile = 'medications.json';

class Medication {
    public $id, $name, $dosage, $interval_hours, $last_taken, $history;

    function __construct($name, $dosage, $interval_hours, $id = 0, $last_taken = null, $history = []) {
        $this->id = $id;
        $this->name = $name;
        $this->dosage = $dosage;
        $this->interval_hours = $interval_hours;
        $this->last_taken = $last_taken ? new DateTime($last_taken) : new DateTime();
        $this->history = array_map(function($t) { return new DateTime($t); }, $history);
    }

    function isDue() {
        $next = clone $this->last_taken;
        $next->modify("+{$this->interval_hours} hours");
        return new DateTime() >= $next;
    }

    function nextDose() {
        $next = clone $this->last_taken;
        $next->modify("+{$this->interval_hours} hours");
        return $next;
    }

    function takeDose() {
        $now = new DateTime();
        $this->last_taken = $now;
        $this->history[] = $now;
    }

    function toArray() {
        return [
            'id' => $this->id,
            'name' => $this->name,
            'dosage' => $this->dosage,
            'interval_hours' => $this->interval_hours,
            'last_taken' => $this->last_taken->format(DateTime::ISO8601),
            'history' => array_map(function($d) { return $d->format(DateTime::ISO8601); }, $this->history)
        ];
    }

    static function fromArray($data) {
        return new self($data['name'], $data['dosage'], $data['interval_hours'], $data['id'], $data['last_taken'], $data['history']);
    }
}

class TimerApp {
    private $medications = [];
    private $file;

    function __construct($file) {
        $this->file = $file;
        $this->load();
    }

    function load() {
        if (file_exists($this->file)) {
            $data = json_decode(file_get_contents($this->file), true);
            foreach ($data as $item) {
                $this->medications[] = Medication::fromArray($item);
            }
        }
    }

    function save() {
        $data = array_map(function($m) { return $m->toArray(); }, $this->medications);
        file_put_contents($this->file, json_encode($data, JSON_PRETTY_PRINT));
    }

    function addMedication($name, $dosage, $interval) {
        $maxId = 0;
        foreach ($this->medications as $m) if ($m->id > $maxId) $maxId = $m->id;
        $med = new Medication($name, $dosage, $interval, $maxId + 1);
        $this->medications[] = $med;
        $this->save();
        echo "💊 Added medication: $name $dosage every $interval" . "h (ID: {$med->id})\n";
    }

    function list() {
        if (empty($this->medications)) {
            echo "No medications.\n";
            return;
        }
        echo "\n📋 Medications:\n";
        foreach ($this->medications as $m) {
            $due = $m->isDue() ? "\033[31m🔴 DUE\033[0m" : "\033[32m⏳ " . $m->nextDose()->format('H:i') . "\033[0m";
            echo "  [{$m->id}] {$m->name} {$m->dosage} – $due\n";
        }
    }

    function takeDose($id) {
        foreach ($this->medications as $m) {
            if ($m->id == $id) {
                $m->takeDose();
                $this->save();
                echo "✅ Took {$m->name} {$m->dosage} at " . (new DateTime())->format('H:i') . "\n";
                return;
            }
        }
        echo "❌ Medication with ID $id not found.\n";
    }

    function checkDue() {
        $due = array_filter($this->medications, function($m) { return $m->isDue(); });
        if (empty($due)) {
            echo "✅ No medications due right now.\n";
            return;
        }
        echo "\n⏰ Due Medications:\n";
        foreach ($due as $m) {
            $overdue = (int)((time() - $m->nextDose()->getTimestamp()) / 60);
            echo "  [{$m->id}] {$m->name} {$m->dosage} – overdue by $overdue min\n";
        }
    }

    function history() {
        if (empty($this->medications)) {
            echo "No medications.\n";
            return;
        }
        echo "\n📜 History (last 5 doses per medication):\n";
        foreach ($this->medications as $m) {
            echo "  {$m->name}:\n";
            $recent = array_slice($m->history, -5);
            if (empty($recent)) {
                echo "    (no history)\n";
            } else {
                foreach ($recent as $dt) {
                    echo "    - " . $dt->format('Y-m-d H:i') . "\n";
                }
            }
        }
    }
}

if ($argc < 2) {
    die("Usage: php medication_timer.php <command> [options]\n");
}
$app = new TimerApp($dataFile);
$cmd = $argv[1];

switch ($cmd) {
    case 'add':
        if ($argc < 5) die("add <name> <dosage> <interval_hours>\n");
        $app->addMedication($argv[2], $argv[3], (float)$argv[4]);
        break;
    case 'list':
        $app->list();
        break;
    case 'take':
        if ($argc < 3) die("take <id>\n");
        $app->takeDose((int)$argv[2]);
        break;
    case 'check':
        $app->checkDue();
        break;
    case 'history':
        $app->history();
        break;
    default:
        echo "Unknown command.\n";
}
?>
