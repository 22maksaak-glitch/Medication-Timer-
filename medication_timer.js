// medication_timer.js
#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const { program } = require('commander');
const chalk = require('chalk');

const DATA_FILE = 'medications.json';

class Medication {
    constructor(name, dosage, intervalHours, id = 0, lastTaken = null, history = []) {
        this.id = id;
        this.name = name;
        this.dosage = dosage;
        this.intervalHours = intervalHours;
        this.lastTaken = lastTaken ? new Date(lastTaken) : new Date();
        this.history = history.map(d => new Date(d));
    }

    isDue() {
        const next = new Date(this.lastTaken.getTime() + this.intervalHours * 3600000);
        return new Date() >= next;
    }

    nextDose() {
        return new Date(this.lastTaken.getTime() + this.intervalHours * 3600000);
    }

    takeDose() {
        const now = new Date();
        this.lastTaken = now;
        this.history.push(now);
    }

    toJSON() {
        return {
            id: this.id,
            name: this.name,
            dosage: this.dosage,
            intervalHours: this.intervalHours,
            lastTaken: this.lastTaken.toISOString(),
            history: this.history.map(d => d.toISOString())
        };
    }

    static fromJSON(data) {
        return new Medication(data.name, data.dosage, data.intervalHours, data.id, data.lastTaken, data.history);
    }
}

class TimerApp {
    constructor() {
        this.medications = [];
        this.load();
    }

    load() {
        if (fs.existsSync(DATA_FILE)) {
            const data = JSON.parse(fs.readFileSync(DATA_FILE));
            this.medications = data.map(m => Medication.fromJSON(m));
        }
    }

    save() {
        fs.writeFileSync(DATA_FILE, JSON.stringify(this.medications.map(m => m.toJSON()), null, 2));
    }

    addMedication(name, dosage, interval) {
        const maxId = this.medications.reduce((max, m) => Math.max(max, m.id), 0);
        const med = new Medication(name, dosage, interval, maxId + 1);
        this.medications.push(med);
        this.save();
        console.log(`💊 Added medication: ${name} ${dosage} every ${interval}h (ID: ${med.id})`);
    }

    list() {
        if (this.medications.length === 0) {
            console.log('No medications.');
            return;
        }
        console.log('\n📋 Medications:');
        for (const m of this.medications) {
            const due = m.isDue() ? chalk.red('🔴 DUE') : chalk.green(`⏳ ${m.nextDose().toTimeString().slice(0,5)}`);
            console.log(`  [${m.id}] ${m.name} ${m.dosage} – ${due}`);
        }
    }

    takeDose(id) {
        const med = this.medications.find(m => m.id === id);
        if (!med) {
            console.log(`❌ Medication with ID ${id} not found.`);
            return;
        }
        med.takeDose();
        this.save();
        console.log(`✅ Took ${med.name} ${med.dosage} at ${new Date().toTimeString().slice(0,5)}`);
    }

    checkDue() {
        const due = this.medications.filter(m => m.isDue());
        if (due.length === 0) {
            console.log('✅ No medications due right now.');
            return;
        }
        console.log('\n⏰ Due Medications:');
        for (const m of due) {
            const overdue = Math.floor((new Date() - m.nextDose()) / 60000);
            console.log(`  [${m.id}] ${m.name} ${m.dosage} – overdue by ${overdue} min`);
        }
    }

    history() {
        if (this.medications.length === 0) {
            console.log('No medications.');
            return;
        }
        console.log('\n📜 History (last 5 doses per medication):');
        for (const m of this.medications) {
            console.log(`  ${m.name}:`);
            const recent = m.history.slice(-5);
            if (recent.length === 0) {
                console.log('    (no history)');
            } else {
                for (const d of recent) {
                    console.log(`    - ${d.toISOString().slice(0,16).replace('T', ' ')}`);
                }
            }
        }
    }
}

program
    .command('add <name> <dosage> <interval>')
    .action((name, dosage, interval) => {
        const app = new TimerApp();
        app.addMedication(name, dosage, parseFloat(interval));
    });

program
    .command('list')
    .action(() => {
        const app = new TimerApp();
        app.list();
    });

program
    .command('take <id>')
    .action((id) => {
        const app = new TimerApp();
        app.takeDose(parseInt(id));
    });

program
    .command('check')
    .action(() => {
        const app = new TimerApp();
        app.checkDue();
    });

program
    .command('history')
    .action(() => {
        const app = new TimerApp();
        app.history();
    });

program.parse(process.argv);
